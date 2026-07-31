# 個人フォークについて

これは [7ka-Hiira/hazkey](https://github.com/7ka-Hiira/hazkey) の個人フォークです。`dev` ブランチに以下の変更が入っています。

| 変更内容 | ブランチ | 上流 |
|----------|----------|------|
| 自動変換の最小文字数を設定可能に | `feat/auto-convert-min-chars` | [PR #32](https://github.com/7ka-Hiira/hazkey/pull/32) |
| Ctrl-H を Backspace 相当に | `feat/ctrl-h` | [PR #31](https://github.com/7ka-Hiira/hazkey/pull/31) |
| 直接変換（Ctrl-U / F6–F10）確定時の学習 | `feat/direct-conversion-learning` | 未送信 |
| 自動変換中の末尾文節ふりがな表示 + ハイライト | `feat-yomigana` | — |
| 文節単位の部分確定・部分キャンセル（Shift+Enter, Shift+Backspace） | `dev` | — |
| `emacs-hazkey`: mozc.el の helper プロトコルによる Emacs クライアント | `dev` | — |
| Shift+Left/Right による文節区切り調整 | `dev` | [PR #28](https://github.com/7ka-Hiira/hazkey/pull/28)（stakeval0 さん作）の cherry-pick |
| 辞書 v3.1.0-beta.15 への更新、Swift 6.3+ ビルド修正、deb パッケージングツール | `dev` | — |

## ブランチ構成

- `main` — 上流 `7ka-Hiira/hazkey` に追従
- `dev` — 常用ブランチ（全変更をマージ）
- `feat/*` — 機能ごとのブランチ（`main` ベース、PR 用）

## パッケージング

- `.deb` パッケージは Actions → "Build .deb packages" → Run workflow で生成できます（artifact からダウンロード。`junktools/hazkey-deb-install.sh` で最新版の取得とインストールができます）
- ローカルでのビルドは clone 後に `junktools/build-deb-local.sh` を実行してください（事前に `sudo junktools/install-deps.sh` で依存パッケージを導入）

---

# fcitx5-hazkey

Hazkey input method for fcitx5

[AzooKeyKanaKanjiConverter](https://github.com/azooKey/AzooKeyKanaKanjiConverter)を利用したIMEです

## ホームページ

[https://hazkey.hiira.dev](https://hazkey.hiira.dev)

## ドキュメント

[https://hazkey.hiira.dev/docs](https://hazkey.hiira.dev/docs)

## インストール

[インストールガイド](https://hazkey.hiira.dev/docs/install)

現在AURと[debianパッケージ](https://github.com/7ka-Hiira/fcitx5-hazkey/releases/latest)が利用できます。

## ビルド

詳細は[ドキュメントのビルドページを参照してください](https://hazkey.hiira.dev/docs/development/build)。

### 依存関係

- Swift >= 6.1
- fcitx5 >= 5.0.4
- Qt >= 6.7 (6.2以降でビルド可能ですが表示が崩れる場合があります)
- CMake >= 3.21 (4.x以降推奨)
- Protobuf >= 3.12
- Ninja
- Gettext

### ソースビルド・インストール手順

ninjaを利用します。

```sh
git clone --recursive https://github.com/7ka-Hiira/hazkey.git
cd hazkey
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DGGML_VULKAN=OFF -G Ninja ..
ninja
sudo ninja install
```

## ライセンス

[MIT License](./LICENSE)
