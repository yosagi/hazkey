#!/usr/bin/env bash
# 目的: hazkey のビルドに必要なシステムパッケージをインストールする
# 関連: reports/project_context.md (ビルド依存の充足状況)
# 前提: Ubuntu 22.04+ / sudo 権限で実行

set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "error: このスクリプトは sudo で実行してください" >&2
    echo "  sudo $0" >&2
    exit 1
fi

# GCC のメジャーバージョンを検出 (libgcc-*-dev 等のパッケージ名に使う)
GCC_VER=$(gcc -dumpversion 2>/dev/null | cut -d. -f1)
if [[ -z "$GCC_VER" ]]; then
    echo "warn: gcc が見つかりません。libgcc-dev / libstdc++-dev は手動で入れてください" >&2
    GCC_DEV_PKGS=""
else
    GCC_DEV_PKGS="libgcc-${GCC_VER}-dev libstdc++-${GCC_VER}-dev"
fi

# ---------- Swift ツールチェーンのシステム依存 ----------
# https://www.swift.org/install/linux/
# swiftly (ユーザーレベル) で Swift 本体を入れる前に必要
SWIFT_DEPS=(
    binutils
    git
    gnupg2
    libc6-dev
    libcurl4-openssl-dev
    libedit2
    libpython3-dev
    libsqlite3-0
    libxml2-dev
    libz3-dev
    pkg-config
    tzdata
    zip
    unzip
    zlib1g-dev
    python3-lldb-13
    $GCC_DEV_PKGS
)

# ---------- hazkey ビルド依存 ----------

# fcitx5 開発ライブラリ
FCITX5_DEPS=(
    libfcitx5core-dev
    libfcitx5config-dev
    libfcitx5utils-dev
    fcitx5-modules-dev
)

# Qt6 (設定GUI: hazkey-settings)
QT6_DEPS=(
    qt6-base-dev
    qt6-tools-dev
    qt6-tools-dev-tools
    qt6-l10n-tools
    linguist-qt6
)

# ビルドツール・その他
BUILD_DEPS=(
    cmake
    ninja-build
    gettext
    libprotobuf-dev
    protobuf-compiler
    libvulkan-dev
    libglx-dev
    libgl1-mesa-dev
    libxkbcommon-dev
    # deb パッケージング用 (build-deb-local.sh)
    dpkg-dev
    file
)

ALL_PKGS=(
    "${SWIFT_DEPS[@]}"
    "${FCITX5_DEPS[@]}"
    "${QT6_DEPS[@]}"
    "${BUILD_DEPS[@]}"
)

echo "=== hazkey ビルド依存パッケージのインストール ==="
echo ""
echo "インストール対象: ${#ALL_PKGS[@]} パッケージ"
echo ""

apt-get update
apt-get install -y "${ALL_PKGS[@]}"

# ---------- Vulkan SDK (glslc 等を含む) ----------
# Ubuntu 22.04 の標準リポジトリには glslc が無いため LunarG リポジトリから取得
# https://vulkan.lunarg.com/sdk/home
. /etc/os-release
if [[ "$VERSION_CODENAME" == "jammy" ]]; then
    echo ""
    echo "=== Vulkan SDK のインストール (Ubuntu 22.04) ==="
    wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add -
    wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list \
        https://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list
    apt-get update
    apt-get install -y vulkan-sdk
else
    # noble (24.04) 以降は標準リポジトリに glslc がある
    echo ""
    echo "=== glslc のインストール (Ubuntu 24.04+) ==="
    apt-get install -y glslc || {
        echo "warn: glslc のインストールに失敗しました。手動でインストールしてください" >&2
        echo "      (無くても GGML_VULKAN=OFF でビルドは可能)" >&2
    }
fi

echo ""
echo "=== 完了 ==="
echo ""
echo "次のステップ:"
echo "  1. Swift のインストール (ユーザー権限で実行):"
echo '     curl -O https://download.swift.org/swiftly/linux/swiftly-$(uname -m).tar.gz && \'
echo '     tar zxf swiftly-$(uname -m).tar.gz && \'
echo '     ./swiftly init --quiet-shell-followup && \'
echo '     . "${SWIFTLY_HOME_DIR:-$HOME/.local/share/swiftly}/env.sh" && \'
echo '     hash -r'
echo "  2. git submodule の取得:"
echo "     git submodule update --init --recursive"
echo "  3. ビルド:"
echo "     mkdir build && cd build"
echo "     cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -G Ninja .."
echo "     ninja"
