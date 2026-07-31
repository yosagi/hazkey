#!/usr/bin/env bash
# 目的: GitHub Actions (build-deb.yml) の最新成功 run から自ホスト向けの deb を
#       ダウンロードしてインストールする。配布先PCで実行する更新スクリプト。
# 関連: .github/workflows/build-deb.yml, reports/tasks/2026-07-31_task_deb_deploy.md
# 前提: gh CLI が認証済み (gh auth login)、sudo 権限、Ubuntu 22.04/24.04/26.04
#       使い方: hazkey-deb-install.sh [branch]   (branch 省略時は dev)

set -euo pipefail

REPO="yosagi/hazkey"
WORKFLOW="build-deb.yml"
BRANCH="${1:-dev}"

command -v gh >/dev/null || { echo "ERROR: gh CLI が見つかりません" >&2; exit 1; }
gh auth status >/dev/null 2>&1 || { echo "ERROR: gh が未認証です。gh auth login を実行してください" >&2; exit 1; }

. /etc/os-release
case "${VERSION_CODENAME:-}" in
    jammy|noble)
        CODENAME="${VERSION_CODENAME}"
        ;;
    *)
        # 26.04 以降は Actions ランナー未提供のため noble の deb を流用する
        echo "NOTE: ${VERSION_CODENAME:-unknown} 向けビルドは無いため noble の deb を使います" >&2
        CODENAME="noble"
        ;;
esac

RUN_ID=$(gh run list --repo "${REPO}" --workflow "${WORKFLOW}" --branch "${BRANCH}" \
    --status success --limit 1 --json databaseId --jq '.[0].databaseId')
[ -n "${RUN_ID}" ] || { echo "ERROR: ${BRANCH} ブランチに成功した ${WORKFLOW} の run がありません" >&2; exit 1; }

RUN_DATE=$(gh run view "${RUN_ID}" --repo "${REPO}" --json createdAt,headSha \
    --jq '"\(.createdAt) (\(.headSha[0:7]))"')
echo "run ${RUN_ID}: ${RUN_DATE} / branch=${BRANCH} / codename=${CODENAME}"

DLDIR=$(mktemp -d)
trap 'rm -rf "${DLDIR}"' EXIT

gh run download "${RUN_ID}" --repo "${REPO}" \
    --name "fcitx5-hazkey_${CODENAME}_amd64" --dir "${DLDIR}"

DEB=$(find "${DLDIR}" -name '*.deb' | head -1)
[ -n "${DEB}" ] || { echo "ERROR: artifact に deb が見つかりません" >&2; exit 1; }

echo "installing: $(basename "${DEB}")"
sudo apt install -y --reinstall "${DEB}"

echo
echo "インストール完了。fcitx5 の再起動（Wayland+KDE ではログアウト→ログイン）で反映されます。"
