#!/usr/bin/env bash
# 目的: deb に同梱する著作権・ライセンス表示 (/usr/share/doc/<package>/copyright) を生成する。
#       本文は各配布元のライセンスファイルをビルド時にそのまま連結する (手で転記しない)。
#       配布元に本文が無いもの (ICU, marisa-trie, 絵文字辞書の出所など) は junktools/deb-copyright/ に置いてある。
# 関連: junktools/make-debs.sh, junktools/deb-copyright/
# 前提: submodule 取得済み。hazkey-server のときはビルド済み (SwiftPM の checkout が要る)
#       使い方: junktools/gen-deb-copyright.sh <pkgroot> <package> [<swiftpm-checkouts-dir>]
#               <package> は hazkey-server / fcitx5-hazkey / ibus-hazkey / emacs-hazkey
#               <swiftpm-checkouts-dir> は hazkey-server のビルドディレクトリ下の swift-build/checkouts
#               (hazkey-server のときだけ要る。第三者のコードをまとめて抱えているのはサーバー側)
#       SwiftPM の依存が増えてライセンスファイルが見つからないときは失敗する。
#       本文を持たない依存は下の EXTRA_NOTICE に補足ファイルを登録する。

set -euo pipefail

if [ $# -lt 2 ] || [ $# -gt 3 ]; then
    echo "usage: $0 <pkgroot> <package> [<swiftpm-checkouts-dir>]" >&2
    exit 1
fi

PKGROOT="$1"
PACKAGE="$2"
CHECKOUTS="${3:-}"
ROOT=$(cd "$(dirname "$0")/.." && pwd)
EXTRA="${ROOT}/junktools/deb-copyright"
RESOLVED="${ROOT}/hazkey-server/Package.resolved"
OUT="${PKGROOT}/usr/share/doc/${PACKAGE}/copyright"

# Swift packages that bundle code whose license text is not in the checkout.
# value: "title|source-url|license-file"
declare -A EXTRA_NOTICE=(
    [swiftymarisa]="marisa-trie (bundled in SwiftyMarisa)|https://github.com/s-yata/marisa-trie|${EXTRA}/marisa-trie-COPYING.md"
)

section() {
    # section <title> <source-url> <files-desc> <license-file>...
    local title="$1" url="$2" files="$3"; shift 3
    printf '\n%s\n' "======================================================================"
    printf '%s\n' "${title}"
    printf 'Source: %s\n' "${url}"
    printf 'Files:  %s\n' "${files}"
    printf '%s\n\n' "----------------------------------------------------------------------"
    local f
    for f in "$@"; do
        [ -f "${f}" ] || { echo "ERROR: license file not found: ${f}" >&2; exit 1; }
        cat "${f}"
        printf '\n'
    done
}

find_license() {
    # prints the license file(s) at the top of a checkout (LICENSE, LICENSE.txt, COPYING.md, NOTICE ...)
    find "$1" -maxdepth 1 -type f \( -iname 'licen[cs]e*' -o -iname 'copying*' -o -iname 'notice*' \) | sort
}

# Package.resolved pins: "identity|location|version-or-revision"
swift_pins() {
    awk -F'"' '
        /"identity"/ { if (id != "") print id "|" loc "|" (ver != "" ? ver : rev); id = $4; loc = ""; rev = ""; ver = "" }
        /"location"/ { loc = $4 }
        /"revision"/ { rev = $4 }
        /"version"/  { if ($4 != "") ver = $4 }
        END          { if (id != "") print id "|" loc "|" (ver != "" ? ver : rev) }
    ' "${RESOLVED}"
}

LIBDIR="/usr/lib/<arch>/hazkey"
HAZKEY_URL="https://github.com/7ka-Hiira/hazkey"

header() {
    cat <<EOF
${PACKAGE} (personal fork build)

This package is built from https://github.com/yosagi/hazkey, a fork of
${HAZKEY_URL}. Each section below reproduces the license text shipped
with that component.
EOF
}

server_notices() {
    [ -d "${CHECKOUTS}" ] || { echo "ERROR: swiftpm checkouts not found: '${CHECKOUTS}' (build hazkey-server first)" >&2; exit 1; }

    cat <<'EOF'

The Swift runtime and Foundation are statically linked into hazkey-server.
They are licensed under the Apache License 2.0 with the Runtime Library
Exception, which waives the attribution requirements for compiled code.
EOF

    section "hazkey (hazkey-server, hazkey-settings)" "${HAZKEY_URL}" \
        "${LIBDIR}/{hazkey-server,hazkey-settings}" \
        "${ROOT}/LICENSE"

    while IFS='|' read -r id loc ver; do
        dir="${CHECKOUTS}/$(basename "${loc}" .git)"
        [ -d "${dir}" ] || { echo "ERROR: checkout not found for ${id}: ${dir}" >&2; exit 1; }
        mapfile -t lic < <(find_license "${dir}")
        [ ${#lic[@]} -gt 0 ] || { echo "ERROR: no license file in ${dir}" >&2; exit 1; }
        section "$(basename "${loc}" .git) ${ver} (Swift package, statically linked)" \
            "${loc}" "${LIBDIR}/hazkey-server" "${lic[@]}"
        if [ -n "${EXTRA_NOTICE[${id}]:-}" ]; then
            IFS='|' read -r xtitle xurl xfile <<<"${EXTRA_NOTICE[${id}]}"
            section "${xtitle}" "${xurl}" "${LIBDIR}/hazkey-server" "${xfile}"
        fi
    done < <(swift_pins)

    section "ICU (via swift-foundation-icu, statically linked)" \
        "https://github.com/unicode-org/icu" \
        "${LIBDIR}/hazkey-server" \
        "${EXTRA}/ICU-76.1-LICENSE"

    section "llama.cpp / ggml" \
        "https://github.com/7ka-hiira/llama.cpp (fork of https://github.com/ggml-org/llama.cpp)" \
        "${LIBDIR}/libllama/" \
        "${ROOT}/hazkey-server/llama.cpp/LICENSE"

    section "azooKey dictionary" \
        "https://github.com/azooKey/azooKey_dictionary_storage" \
        "/usr/share/hazkey/Dictionary/" \
        "${ROOT}/hazkey-server/azooKey_dictionary_storage/LICENSE"

    section "azooKey emoji dictionary" \
        "https://github.com/azooKey/azooKey_emoji_dictionary_storage" \
        "/usr/share/hazkey/emoji_all_E16.0.txt" \
        "${EXTRA}/emoji-dictionary.txt" "${EXTRA}/mozc-LICENSE" "${EXTRA}/Unicode-3.0.txt"
}

mkdir -p "$(dirname "${OUT}")"
{
    header
    case "${PACKAGE}" in
        hazkey-server)
            server_notices
            ;;
        fcitx5-hazkey)
            section "hazkey (fcitx5-hazkey)" "${HAZKEY_URL}" "fcitx5 addon" "${ROOT}/LICENSE"
            ;;
        ibus-hazkey)
            section "hazkey (ibus-hazkey)" "${HAZKEY_URL}" "${LIBDIR}/ibus-engine-hazkey" "${ROOT}/LICENSE"
            ;;
        emacs-hazkey)
            section "hazkey (emacs-hazkey)" "${HAZKEY_URL}" "${LIBDIR}/hazkey_emacs_helper" "${ROOT}/LICENSE"
            section "Mozc emacs helper library (in hazkey_emacs_helper)" \
                "https://github.com/google/mozc" \
                "${LIBDIR}/hazkey_emacs_helper" \
                "${ROOT}/emacs-hazkey/mozc_lib/LICENSE"
            ;;
        *)
            echo "ERROR: unknown package: ${PACKAGE}" >&2
            exit 1
            ;;
    esac
} > "${OUT}"

chmod 644 "${OUT}"
echo "generated ${OUT}"
