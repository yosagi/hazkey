# 構成

hazkey は、変換を行うサーバーと、入力メソッドの枠組みごとのクライアントからなります。
キー操作の振る舞いはクライアントごとに書かず、共通のライブラリ `hazkey-frontend-core` に置いています。

```
  fcitx5            ibus             Emacs (mozc.el)
    │                 │                    │
fcitx5-hazkey     ibus-hazkey        hazkey_emacs_helper     ← クライアント（枠組みへの橋渡しだけ）
    └───────── hazkey-frontend-core ───────┘                 ← 状態機械・キー表・サーバー接続
                      │  UNIX ソケット + protobuf
                hazkey-server                                ← 変換・設定・学習データ
                      │
              hazkey-settings（設定 GUI。core の接続部を使う）
```

## 各部の役割

### hazkey-server（`hazkey-server/`、Swift）

- AzooKeyKanaKanjiConverter による変換と、Zenzai（llama.cpp）による AI 変換
- 設定ファイル `config.json` と学習データの読み書き。書き手はサーバーだけ
- 1ユーザーに1つ。`$XDG_RUNTIME_DIR/hazkey-server.<uid>.sock` で複数の接続を受け付けます。入力途中の状態（未確定の文字列や候補）は接続ごとに持ち、変換器・設定・学習データは共有します
- キーバインドは Profile の `keyBindings` を保存してクライアントに配るだけで、中身は解釈しません

### hazkey-frontend-core（`hazkey-frontend-core/`、C++ 静的ライブラリ）

- 状態機械 `StateMachine`: キーを受けて、preedit・候補ウィンドウ・確定文字列を `Output` として返します。部分確定、注目文節の退避、ふりがなの判定などもここにあります
- キー表 `KeyBindings`: 操作の一覧と既定のキー、キーの表記を持ちます。サーバーから受け取った `keyBindings` を既定の表に重ねて使います（[キーバインド](keybindings.md)）
- サーバー接続 `ServerConnection` / `ServerClient`: ソケット通信、命令の組み立て、つながらないときの再試行

### クライアント

core の結果をそれぞれの枠組みの表示に写すアダプターです。クライアントが自前で持つのは次のものだけです。

- サーバーの起動方法とログの出し先（`ConnectionHooks`）
- 周辺テキストの取得など、枠組みに問い合わせること（`FrontendHooks`）
- キーイベントの変換と、preedit・候補の表示

| クライアント | ディレクトリ | 枠組みでの位置づけ |
|-------------|-------------|------------------|
| fcitx5 用 hazkey エンジン | `fcitx5-hazkey/` | fcitx5 のアドオン（`InputMethodEngineV2`） |
| ibus 用 hazkey エンジン | `ibus-hazkey/` | ibus のエンジン（`IBusEngine`）。ibus-daemon が起動する |
| hazkey_emacs_helper | `emacs-hazkey/` | mozc.el が起動する helper プロセス。`mozc_emacs_helper` 互換 |

枠組みの作法の違いはアダプターで吸収します。たとえば ibus はフォーカスが外れると preedit をそのまま確定するので、ibus 用エンジンでは preedit に確定される文字列だけを置き、ふりがなは補助テキストに出しています。

### 通信プロトコル（`protocol/`）

`base.proto`（要求と応答の封筒）、`commands.proto`（入力・変換の命令）、`config.proto`（設定）。
C++ 側はビルド時に生成し、Swift 側の `.pb.swift` は生成済みのものをリポジトリに置いています。

## 起動とつながり方

1. クライアントが最初の入力でサーバーにつなぎます。ソケットが無ければ、`ConnectionHooks::startServer` でサーバーを起動してつなぎ直します
2. 入力を始めるたびに、キーバインドをサーバーから取り直します。サーバーは `config.json` が変わっていたときだけ読み直すので、手で書き換えた設定も再起動なしで効きます
3. 学習データはサーバーが更新するので、どのクライアントで確定した結果もすべてのクライアントに効きます

## パッケージ

deb は `hazkey-server`（サーバーと hazkey-settings）、`fcitx5-hazkey`、`ibus-hazkey`、`emacs-hazkey` の4つに分かれています。
パッケージングは `junktools/make-debs.sh` が行い、CI（`.github/workflows/build-deb.yml`）とローカルビルド（`junktools/build-deb-local.sh`）のどちらも、これを呼んでいます。
