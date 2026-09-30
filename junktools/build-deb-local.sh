#!/usr/bin/env bash
# 目的: clone 直後のリポジトリからローカルで deb パッケージ一式
#       (hazkey-server, fcitx5-hazkey, ibus-hazkey, emacs-hazkey) をビルドする。
#       CI (.github/workflows/build-deb.yml) と同じ手順のローカル版。
# 関連: .github/workflows/build-deb.yml, junktools/install-deps.sh, junktools/make-debs.sh
# 前提: junktools/install-deps.sh を sudo で実行済み、Swift 6.1+ が PATH にある
#       使い方: junktools/build-deb-local.sh [version]
#               (version 省略時はコミット時刻+SHA を自動生成)
#       環境変数: GGML_VULKAN=ON/OFF (省略時は glslc の有無で自動判定)
#                 HAZKEY_LTO=full/none (省略時 full。none にするとリンクが速い)
#                 DEB_MAINTAINER (省略時は git の user.name / user.email)

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
    # コミット時刻 (UTC) を入れる。日付+SHA だけだと同じ日のビルド同士の順序が
    # SHA の数字部分で決まり、新しいビルドがダウングレード扱いになることがある
    VER="$(TZ=UTC git -C "${ROOT}" log -1 --format=%cd --date=format-local:%Y%m%d.%H%M%S)+g$(git -C "${ROOT}" rev-parse --short HEAD)"
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

if [ -z "${DEB_MAINTAINER:-}" ]; then
    DEB_MAINTAINER="$(git -C "${ROOT}" config user.name) <$(git -C "${ROOT}" config user.email)>"
fi
export DEB_MAINTAINER

BUILDROOT="${ROOT}/tmp/deb-build"
STAGE="${BUILDROOT}/stage"
OUTDIR="${ROOT}/tmp/debs"
mkdir -p "${BUILDROOT}"
rm -rf "${STAGE}" "${BUILDROOT}/pkgroot"

echo "=== hazkey deb build ==="
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
# build_component <source-dir> <package> [cmake args...]
# installs into ${STAGE}/<package>, which becomes one deb
build_component() {
    local src="$1" pkg="$2"; shift 2
    local bld="${BUILDROOT}/$(basename "${src}")"
    cmake -S "${ROOT}/${src}" -B "${bld}" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -G Ninja "$@"
    ninja -C "${bld}" -j"$(nproc)"
    DESTDIR="${STAGE}/${pkg}" ninja -C "${bld}" install
}

build_component hazkey-settings hazkey-server
build_component hazkey-server hazkey-server \
    -DGGML_VULKAN="${GGML_VULKAN}" \
    -DHAZKEY_SERVER_SWIFT_LTO_MODE="${HAZKEY_LTO}"
build_component fcitx5-hazkey fcitx5-hazkey
build_component ibus-hazkey ibus-hazkey
build_component emacs-hazkey emacs-hazkey

restore_ui
trap - EXIT

# ---------- deb パッケージング (CI と同じ) ----------
"${ROOT}/junktools/make-debs.sh" "${STAGE}" "${OUTDIR}" "${VER}" "${CODENAME}" \
    "${BUILDROOT}/hazkey-server/swift-build/checkouts"

echo ""
echo "=== 完了 ==="
ls -1 "${OUTDIR}"/*_"${VER}"_"${CODENAME}"_amd64.deb
echo ""
echo "インストール (hazkey-server と使うクライアントを一緒に入れる):"
echo "  sudo apt install -y --reinstall ${OUTDIR}/{hazkey-server,fcitx5-hazkey,emacs-hazkey}_${VER}_${CODENAME}_amd64.deb"
echo "  pkill -x hazkey-server   # 古いサーバーを止める (クライアントが新しいサーバーを起動し直す)"
