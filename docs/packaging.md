# deb パッケージの作り方

このフォークには、Ubuntu 向けの deb を作る仕組みがあります。GitHub Actions で作る方法と、手元で作る方法のどちらでも、同じ4つのパッケージができます。

| パッケージ | 中身 |
|------------|------|
| `hazkey-server` | 変換サーバーと設定ツール `hazkey-settings` |
| `fcitx5-hazkey` | fcitx5 用 hazkey エンジン |
| `ibus-hazkey` | ibus 用 hazkey エンジン |
| `emacs-hazkey` | Emacs 用の `hazkey_emacs_helper` |

クライアントのパッケージは、同じ版の `hazkey-server` に依存します。それぞれの使い方は[使い方](clients.md)を参照してください。

## GitHub Actions で作る

ワークフローは手動実行なので、自分のアカウントにフォークしたリポジトリで実行します。

1. このリポジトリをフォークし、フォーク先の「Actions」タブでワークフローを有効にします
2. 「Build .deb packages」を選び、「Run workflow」でブランチ（通常は `dev`）を選んで実行します。version を空にすると、コミット時刻と SHA から自動で決まります
3. 完了した run のページの「Artifacts」から、使う Ubuntu の版の deb をダウンロードします

Ubuntu 22.04（jammy）、24.04（noble）、26.04（resolute）向けの3つが作られます。artifact は一定期間（既定では90日）で消えます。

## 手元で作る

対応する版の Ubuntu で、clone したリポジトリから実行します。

```sh
sudo junktools/install-deps.sh      # ビルドに必要なパッケージを入れる
junktools/build-deb-local.sh        # Swift 6.1 以降が PATH にあること
```

Swift は別途 [swift.org](https://www.swift.org/install/linux/) の手順で入れてください。
主な環境変数は `GGML_VULKAN=ON/OFF`（Zenzai の Vulkan 対応。省略時は glslc の有無で決まる）と `HAZKEY_LTO=none`（リンクが速くなる）です。

## インストール

使うクライアントのパッケージを、`hazkey-server` と一緒に入れます。

```sh
sudo apt install ./hazkey-server_*.deb ./fcitx5-hazkey_*.deb
```

更新したあとは、古いサーバーを止めてください（`pkill -x hazkey-server`）。次に入力したとき、新しいサーバーが起動します。
上流の、サーバーまで入った `fcitx5-hazkey` の deb からは、そのまま上書きで入れ替えられます。
