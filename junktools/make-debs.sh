#!/usr/bin/env bash
# 目的: パッケージごとに分けてインストールしたツリーから deb を作る
#       (strip、著作権表示、共有ライブラリ依存の検出、control、dpkg-deb)。CI とローカルビルドで共通
# 関連: .github/workflows/build-deb.yml, junktools/build-deb-local.sh, junktools/gen-deb-copyright.sh
# 前提: <stageroot>/<package>/ にそれぞれ DESTDIR でインストール済み。dpkg-dev, file, binutils
#       使い方: junktools/make-debs.sh <stageroot> <outdir> <version> <codename> <swiftpm-checkouts-dir>
#               <stageroot> にあるパッケージだけを作る (hazkey-server / fcitx5-hazkey / ibus-hazkey / emacs-hazkey)
#       環境変数: DEB_MAINTAINER (必須、"Name <email>" の形)

set -euo pipefail

if [ $# -ne 5 ]; then
    echo "usage: $0 <stageroot> <outdir> <version> <codename> <swiftpm-checkouts-dir>" >&2
    exit 1
fi
STAGE=$(realpath "$1")
OUTDIR="$2"
VER="$3"
CODENAME="$4"
CHECKOUTS="$5"
: "${DEB_MAINTAINER:?set DEB_MAINTAINER to \"Name <email>\"}"
ROOT=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "${OUTDIR}"
OUTDIR=$(realpath "${OUTDIR}")

# control fields of each package. the server used to be in fcitx5-hazkey, and
# so was the emacs helper: Replaces/Breaks let them take over those files.
control_fields() {
    local pkg="$1"
    case "${pkg}" in
        hazkey-server)
            cat <<EOF
Replaces: fcitx5-hazkey (<< ${VER})
Breaks: fcitx5-hazkey (<< ${VER})
Description: Hazkey - Japanese input method, conversion server
 Conversion server of Hazkey using AzooKeyKanaKanjiConverter, with
 optional AI-powered conversion via Zenzai (llama.cpp). Also contains
 hazkey-settings. Used by the fcitx5, ibus and Emacs clients.
EOF
            ;;
        fcitx5-hazkey)
            cat <<EOF
Description: Hazkey - Japanese input method for fcitx5
 fcitx5 addon of Hazkey. The conversion is done by hazkey-server.
EOF
            ;;
        ibus-hazkey)
            cat <<EOF
Description: Hazkey - Japanese input method for ibus
 ibus engine of Hazkey. The conversion is done by hazkey-server.
EOF
            ;;
        emacs-hazkey)
            cat <<EOF
Replaces: fcitx5-hazkey (<< ${VER})
Breaks: fcitx5-hazkey (<< ${VER})
Description: Hazkey - Japanese input method for Emacs
 Helper process that lets mozc.el use Hazkey. The conversion is done by
 hazkey-server.
EOF
            ;;
        *)
            echo "ERROR: unknown package: ${pkg}" >&2
            exit 1
            ;;
    esac
}

# dependencies besides the shared libraries
extra_depends() {
    case "$1" in
        hazkey-server) echo "" ;;
        ibus-hazkey) echo "hazkey-server (= ${VER}), ibus" ;;
        *) echo "hazkey-server (= ${VER})" ;;
    esac
}

make_deb() {
    local pkg="$1"
    local pkgroot="${STAGE}/${pkg}"

    if [ "${pkg}" = hazkey-server ]; then
        "${ROOT}/junktools/gen-deb-copyright.sh" "${pkgroot}" "${pkg}" "${CHECKOUTS}"
    else
        "${ROOT}/junktools/gen-deb-copyright.sh" "${pkgroot}" "${pkg}"
    fi

    find "${pkgroot}/usr" -type f -print0 \
        | xargs -0 file -i \
        | grep -E "application/(x-pie-executable|x-sharedlib)" \
        | cut -d: -f1 \
        | xargs -r -I{} strip "{}"

    # dpkg-shlibdeps wants debian/control of the package in the current dir.
    # it warns about the private libraries (llama.cpp) and skips them
    local work
    work=$(mktemp -d)
    mkdir -p "${work}/debian"
    printf 'Source: hazkey\n\nPackage: %s\nArchitecture: amd64\n' "${pkg}" > "${work}/debian/control"
    local elfs=()
    mapfile -t elfs < <(find "${pkgroot}/usr" -type f -print0 | xargs -0 file | grep ELF | cut -d: -f1)
    local shlibs=""
    if [ ${#elfs[@]} -gt 0 ]; then
        shlibs=$(cd "${work}" && dpkg-shlibdeps --warnings=0 -O "${elfs[@]}" 2>/dev/null \
            | sed -n 's/^shlibs:Depends=//p')
        [ -n "${shlibs}" ] || { echo "ERROR: dpkg-shlibdeps found no dependencies for ${pkg}" >&2; exit 1; }
    fi
    rm -rf "${work}"

    local depends extra
    extra=$(extra_depends "${pkg}")
    depends="${shlibs}"
    if [ -n "${extra}" ]; then
        depends="${depends:+${depends}, }${extra}"
    fi

    local size
    size=$(du -sk "${pkgroot}" --exclude=DEBIAN | cut -f1)

    mkdir -p "${pkgroot}/DEBIAN"
    {
        cat <<EOF
Package: ${pkg}
Version: ${VER}
Architecture: amd64
Installed-Size: ${size}
Depends: ${depends}
Maintainer: ${DEB_MAINTAINER}
EOF
        control_fields "${pkg}"
    } > "${pkgroot}/DEBIAN/control"

    dpkg-deb --root-owner-group --build "${pkgroot}" \
        "${OUTDIR}/${pkg}_${VER}_${CODENAME}_amd64.deb"
}

found=0
for pkg in hazkey-server fcitx5-hazkey ibus-hazkey emacs-hazkey; do
    if [ -d "${STAGE}/${pkg}" ]; then
        make_deb "${pkg}"
        found=1
    fi
done
[ "${found}" = 1 ] || { echo "ERROR: no package tree in ${STAGE}" >&2; exit 1; }
