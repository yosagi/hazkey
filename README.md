# 個人フォークについて

これは [7ka-Hiira/hazkey](https://github.com/7ka-Hiira/hazkey) の個人フォークです。私（yosagi）が自分で使うために変更を加えたもので、上流とは別に管理しています。このフォークについての報告や問い合わせは上流に送らないでください。

`dev` ブランチに以下の変更が入っています。

| 変更内容 | ブランチ | 上流 |
|----------|----------|------|
| 自動変換の最小文字数を設定可能に | `feat/auto-convert-min-chars` | [PR #32](https://github.com/7ka-Hiira/hazkey/pull/32) |
| 直接変換（Ctrl-U / F6–F10）確定時の学習 | `feat/direct-conversion-learning` | 未送信 |
| 自動変換中の末尾文節ふりがな表示 + ハイライト（表示の on/off は設定可。fcitx5 ではふりがなは確定されない） | `feat-yomigana`, `dev` | — |
| 文節単位の部分確定・部分キャンセル（Shift+Enter, Shift+Backspace） | `dev` | — |
| `emacs-hazkey`: Emacs 用の `hazkey_emacs_helper`（mozc.el から mozc_emacs_helper の代わりに使う） | `dev` | — |
| `ibus-hazkey`: ibus 用 hazkey エンジン | `dev` | — |
| hazkey-server のマルチクライアント化（全クライアントが1つのサーバーを共有） | `dev` | [issue #39](https://github.com/7ka-Hiira/hazkey/issues/39) |
| キー操作の共通ライブラリ化（`hazkey-frontend-core`） | `dev` | [issue #39](https://github.com/7ka-Hiira/hazkey/issues/39) |
| [キー割り当ての設定](docs/keybindings.md)（hazkey-settings の「キー操作」タブ、または設定ファイルの `keyBindings`） | `dev` | — |
| 入力中のカーソル移動（変換結果の上を要素単位で動く / 読み表示に切り替えて1文字単位、Home / End） | `dev` | — |
| 読みの履歴（確定・取り消しで消えた未確定の読みをキーで戻す、`idle.restore_reading`） | `dev` | — |
| Shift+Left/Right による文節区切り調整 | `dev` | [PR #28](https://github.com/7ka-Hiira/hazkey/pull/28)（stakeval0 さん作）の cherry-pick |
| 辞書 v3.1.0-beta.15 への更新、Swift 6.3+ ビルド修正、[deb パッケージングツール](docs/packaging.md) | `dev` | — |

以前は個別の変更として `dev` に入れていたもので、今は別の変更に吸収されているものです（記録のために残しています）。

| 変更内容 | ブランチ | 上流 | 今の扱い |
|----------|----------|------|----------|
| Ctrl-H を Backspace 相当に | `feat/ctrl-h` | [PR #31](https://github.com/7ka-Hiira/hazkey/pull/31) | キー割り当ての設定の既定値 |

## バイナリ

[Releases](https://github.com/yosagi/hazkey/releases) の deb は、私が自分の PC に入れるための非公式ビルドです（pre-release）。上流の公式な配布物は [7ka-Hiira/hazkey の Releases](https://github.com/7ka-Hiira/hazkey/releases) にあります。

## ドキュメント

- [使い方（fcitx5 / ibus / Emacs）](docs/clients.md)
- [キーバインド](docs/keybindings.md)
- [deb パッケージの作り方](docs/packaging.md)
- [構成](docs/architecture.md)

## ブランチ構成

- `main` — 上流 `7ka-Hiira/hazkey` に追従
- `dev` — 常用ブランチ（全変更をマージ）
- `feat/*` — 機能ごとのブランチ（`main` ベース、PR 用）

---

> **ここから下は上流 [7ka-Hiira/hazkey](https://github.com/7ka-Hiira/hazkey) の README です（内容は変更していません）。インストール手順やドキュメントのリンク先は上流版についてのものです。**

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
