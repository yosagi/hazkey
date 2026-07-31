#!/usr/bin/env bash
# 目的: clone 直後のリポジトリからローカルで deb パッケージをビルドする。
#       CI (.github/workflows/build-deb.yml) と同じ手順のローカル版。
# 関連: .github/workflows/build-deb.yml, junktools/install-deps.sh,
#       reports/tasks/2026-07-31_task_deb_deploy.md
# 前提: junktools/install-deps.sh を sudo で実行済み、Swift 6.1+ が PATH にある
#       使い方: junktools/build-deb-local.sh [version]
#               (version 省略時は日付+SHA を自動生成)
#       環境変数: GGML_VULKAN=ON/OFF (省略時は glslc の有無で自動判定)
#                 HAZKEY_LTO=full/none (省略時 full。none にするとリンクが速い)

set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)

# ---------- 前提チェック ----------
missing=()
for cmd in swift cmake ninja protoc dpkg-shlibdeps dpkg-deb file strip msgfmt; do
    command -v "$cmd" >/dev/null || missing+=("$cmd")
done
if [ ${#missing[@]} -gt 0 ]; then
    echo "ERROR: 次のコマンドが見つかりません: ${missing[*]}" >&2
    echo "  apt パッケージ: sudo junktools/install-deps.sh" >&2
    echo "  Swift: install-deps.sh 完了時に表示される swiftly の手順を参照" >&2
    exit 1
fi

# ---------- パラメータ ----------
VER="${1:-}"
if [ -z "${VER}" ]; then
    VER="$(date +%Y%m%d)+g$(git -C "${ROOT}" rev-parse --short HEAD)"
fi

. /etc/os-release
CODENAME="${VERSION_CODENAME:-local}"

if [ -z "${GGML_VULKAN:-}" ]; then
    if command -v glslc >/dev/null; then
        GGML_VULKAN=ON
    else
        echo "NOTE: glslc が無いため GGML_VULKAN=OFF でビルドします (Zenzai は CPU 推論)" >&2
        GGML_VULKAN=OFF
    fi
fi
HAZKEY_LTO="${HAZKEY_LTO:-full}"

BUILDROOT="${ROOT}/tmp/deb-build"
PKGROOT="${BUILDROOT}/pkgroot"
mkdir -p "${BUILDROOT}"
rm -rf "${PKGROOT}"

echo "=== fcitx5-hazkey deb build ==="
echo "  version:     ${VER}"
echo "  codename:    ${CODENAME}"
echo "  GGML_VULKAN: ${GGML_VULKAN}"
echo "  LTO:         ${HAZKEY_LTO}"
echo ""

# ---------- submodule ----------
git -C "${ROOT}" submodule update --init --recursive

# ---------- jammy の古い uic 対策 (CI と同じ) ----------
# Qt 6.2 の uic は Qt::Orientation:: のスコープ付き enum を解釈できない。
# ソースツリーを汚さないよう、ビルド後に必ず元へ戻す。
UI_FILE="${ROOT}/hazkey-settings/mainwindow.ui"
UI_BACKUP=""
restore_ui() {
    if [ -n "${UI_BACKUP}" ] && [ -f "${UI_BACKUP}" ]; then
        mv "${UI_BACKUP}" "${UI_FILE}"
    fi
}
trap restore_ui EXIT
if [ "${CODENAME}" = "jammy" ] && grep -q "Qt::Orientation::" "${UI_FILE}"; then
    UI_BACKUP="${BUILDROOT}/mainwindow.ui.orig"
    cp "${UI_FILE}" "${UI_BACKUP}"
    sed -i "s/Qt::Orientation::/Qt::/g" "${UI_FILE}"
fi

# ---------- 各コンポーネントのビルド ----------
# ビルドディレクトリは使い回す (二回目以降は差分ビルド)。
# 普段の開発用ビルド (fcitx5-hazkey/build, トップレベル build/) とは分離してある。
build_component() {
    local src="$1"; shift
    local bld="${BUILDROOT}/$(basename "${src}")"
    cmake -S "${ROOT}/${src}" -B "${bld}" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -G Ninja "$@"
    ninja -C "${bld}" -j"$(nproc)"
    DESTDIR="${PKGROOT}" ninja -C "${bld}" install
}

build_component hazkey-settings
build_component fcitx5-hazkey
build_component emacs-hazkey
build_component hazkey-server \
    -DGGML_VULKAN="${GGML_VULKAN}" \
    -DHAZKEY_SERVER_SWIFT_LTO_MODE="${HAZKEY_LTO}"

restore_ui
trap - EXIT

# ---------- strip (CI と同じ) ----------
find "${PKGROOT}/usr" -type f -print0 \
    | xargs -0 file -i \
    | grep -E "application/(x-pie-executable|x-sharedlib)" \
    | cut -d: -f1 \
    | xargs -r -I{} strip "{}"

# ---------- deb パッケージング (CI と同じ) ----------
cd "${BUILDROOT}"
mkdir -p debian
printf 'Source: fcitx5-hazkey\n\nPackage: fcitx5-hazkey\nArchitecture: amd64\n' > debian/control

mapfile -t ELFS < <(find "${PKGROOT}/usr" -type f -print0 | xargs -0 file | grep ELF | cut -d: -f1)
DEPS=$(dpkg-shlibdeps --warnings=0 -O "${ELFS[@]}" 2>/dev/null \
    | sed 's/shlibs:Depends=//' || true)
if [ -z "${DEPS}" ]; then
    DEPS="fcitx5 (>= 5.0.4)"
fi

INSTALLED_SIZE=$(du -sk "${PKGROOT}" --exclude=DEBIAN | cut -f1)

mkdir -p "${PKGROOT}/DEBIAN"
cat > "${PKGROOT}/DEBIAN/control" << EOF
Package: fcitx5-hazkey
Version: ${VER}
Architecture: amd64
Installed-Size: ${INSTALLED_SIZE}
Depends: ${DEPS}
Maintainer: yosagi <yoshida@furo.org>
Description: Hazkey - Japanese input method for fcitx5
 Japanese input method engine using AzooKeyKanaKanjiConverter
 with optional AI-powered conversion via Zenzai (llama.cpp).
EOF

OUT="${ROOT}/tmp/fcitx5-hazkey_${VER}_${CODENAME}_amd64.deb"
dpkg-deb --root-owner-group --build "${PKGROOT}" "${OUT}"

echo ""
echo "=== 完了 ==="
echo "  ${OUT}"
echo ""
echo "インストール:"
echo "  sudo apt install -y --reinstall ${OUT}"
