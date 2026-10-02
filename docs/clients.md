# 使い方（fcitx5 / ibus / Emacs）

hazkey は、変換を行う `hazkey-server` と、入力の受け口になる次の3つからなります。
どれも同じサーバーにつながるので、設定（hazkey-settings）と学習データは共通です。

| パッケージ | 中身 |
|------------|------|
| `hazkey-server` | 変換サーバーと設定ツール `hazkey-settings`。必ず入れる |
| `fcitx5-hazkey` | fcitx5 用 hazkey エンジン |
| `ibus-hazkey` | ibus 用 hazkey エンジン |
| `emacs-hazkey` | Emacs 用の `hazkey_emacs_helper`（mozc_emacs_helper 互換） |

使うものを `hazkey-server` と一緒にインストールしてください。deb の作り方は[deb パッケージの作り方](packaging.md)を参照してください。

## サーバーについて

- サーバーは1ユーザーに1つで、fcitx5 / ibus / Emacs から同時に使えます。入力途中の状態は接続ごとに分かれています
- 自分で起動する必要はありません。エンジンや helper が、サーバーが無ければ起動します
- パッケージを更新したら、古いサーバーを止めてください（`pkill -x hazkey-server`）。入力を始めると新しいサーバーが起動します。fcitx5 や ibus を再起動しても、サーバーは止まりません
- 設定は `~/.config/hazkey/config.json`、学習データは `~/.local/state/hazkey/` に置かれます（`XDG_CONFIG_HOME` / `XDG_STATE_HOME` を設定している場合はその下）

## fcitx5 用 hazkey エンジン（fcitx5-hazkey）

fcitx5 のアドオンとして読み込まれます。

1. インストール後、fcitx5 を再起動します（`fcitx5 -r`）。KDE の Wayland セッションなど、fcitx5 をデスクトップ側が起動している環境では、ログインし直してください
2. fcitx5 の設定（`fcitx5-configtool`）の「入力メソッド」で「Hazkey」を追加します

## ibus 用 hazkey エンジン（ibus-hazkey）

ibus の用語ではエンジンです。ibus-daemon が `ibus-engine-hazkey` を起動します。

1. インストール後、ibus を再起動します（`ibus restart`）
2. `ibus-setup` の「入力メソッド」タブで「追加」を押し、「日本語」の中から「Hazkey」を選びます（GNOME では「設定」→「キーボード」の入力ソース）。コマンドで切り替えるなら `ibus engine hazkey`
3. 「設定」ボタンからは hazkey-settings が開きます

fcitx5 との違い:

- ibus では、フォーカスが外れたときやカーソルを動かしたときに、表示中の preedit がそのまま確定されます。末尾の文節のふりがな `[よみ]` が出ていれば、それも一緒に確定されます。気になる場合は、hazkey-settings の「入力 UI」で「末尾の文節にふりがなを付ける」をオフにしてください（fcitx5 では、ふりがなは確定されません）
- 注目文節の下線と色は、アプリによって出ないことがあります（konsole など）。XIM 経由のアプリ（xterm など）では、preedit がアプリの中ではなく ibus のウィンドウに出ます

### 応用: xpra の中で使う

xpra はセッションの中で ibus-daemon を起動するので、ibus 用 hazkey エンジンがそのまま使えます。
ただし xpra は既定で `--panel=disable` を付けて起動し、候補ウィンドウが出ません。起動コマンドを差し替えて xpra を起動してください。

```sh
XPRA_IBUS_DAEMON_COMMAND="ibus-daemon --xim --verbose --replace --desktop=xpra" \
  xpra start --input-method=ibus ...
```

セッションの中で `ibus engine hazkey` を実行すると切り替わります。候補ウィンドウがカーソルから離れた位置に出ることがあります。

## Emacs 用 hazkey_emacs_helper（emacs-hazkey）

mozc.el は、変換を `mozc_emacs_helper` という別プロセスに任せています。`hazkey_emacs_helper` は同じプロトコルを話すので、mozc.el から mozc の代わりに使えます。

1. mozc.el を用意します（MELPA の `mozc` パッケージ、または Ubuntu の `emacs-mozc`。Doom Emacs なら `packages.el` に `(package! mozc)`）
2. 設定ファイルに次を書きます

```elisp
(require 'mozc)
(setq default-input-method "japanese-mozc")
(setq mozc-helper-program-name "hazkey_emacs_helper")
```

キー操作は fcitx5 / ibus と同じです（[キーバインド](keybindings.md)の設定も共通です）。
helper のログは `$XDG_RUNTIME_DIR/hazkey-emacs/hazkey_emacs_helper.log` に出ます。
