# キーバインド

入力中と候補選択中のキー操作は、設定ファイルで割り当てを変えられます。
設定は fcitx5 / ibus / Emacs で共通です。GUI（hazkey-settings）での編集にはまだ対応していません。

IME のオン・オフなど、fcitx5 や ibus 自身が扱うキーはここでは変えられません。それぞれの設定で変更してください。

## 設定のしかた

`~/.config/hazkey/config.json` は Profile の配列です。先頭の Profile に `keyBindings` を書きます。

```json
[
  {
    "profileName": "...",
    "keyBindings": [
      { "action": "composing.commit", "keys": ["Return", "Control+m"] },
      { "action": "convert_to.katakana_half", "keys": [] }
    ]
  }
]
```

- 書いた操作だけが置き換わります。書いていない操作は既定のキーのままです
- `keys` を空にすると、その操作は無効になります
- 一つの操作に複数のキーを割り当てられます
- 保存すると、次に入力を始めたときから効きます。サーバーやエンジンの再起動は要りません
- 知らない操作名や読めないキーは無視され、ログに出ます
- hazkey-settings で設定を保存しても `keyBindings` は残ります（項目の並び順は変わります）

## キーの書き方

`修飾+修飾+キー名` の形で書きます。

- 修飾: `Control`（`Ctrl` も可）、`Shift`、`Alt`
- キー名: 印字できる ASCII 文字1つ（`a`、`1`、`/` など）、または次の名前。大文字小文字は区別しません
  - `space` `plus` `BackSpace` `Tab` `Return` `Escape` `Delete` `Insert`
  - `Home` `End` `Page_Up` `Page_Down` `Left` `Up` `Right` `Down`
  - `Muhenkan` `Henkan`（`Henkan_Mode` も可）`Hiragana_Katakana` `Zenkaku_Hankaku` `Eisu_toggle`
  - `F1` 〜 `F12`
- 英字は大文字と小文字を区別しません。Shift を押すなら `Shift+a` と書きます
- 修飾は完全に一致したときだけ当たります。`Return` に割り当てても `Control+Return` では効きません。効かせたいときは、そのキーも書き足してください

## 操作の一覧と既定のキー

入力中（未確定の文字列がある状態）:

| 操作 | 既定のキー | 内容 |
|------|-----------|------|
| `composing.commit` | Return | 確定する |
| `composing.commit_prefix` | Shift+Return | 注目文節（ハイライト）より前だけを確定する |
| `composing.shelve_trailing_clause` | Shift+BackSpace, Control+Shift+h | 注目文節を退避して消す。続けて `composing.commit` で確定すると退避した読みが戻り、文字を入力するか取り消すと捨てられる |
| `composing.delete_left` | BackSpace, Control+h | 1文字消す |
| `composing.delete_right` | Delete | カーソルの右を1文字消す |
| `composing.cancel` | Escape | 入力を取り消す |
| `composing.convert` | space, Henkan | 変換して候補を出す |
| `composing.insert_space` | Shift+space | 空白を入力する |
| `composing.focus_candidates` | Up, Down, Tab, Shift+Tab | 候補一覧に入る |
| `composing.cursor_left` | Left | カーソルを左へ |
| `composing.cursor_right` | Right | カーソルを右へ（左へ動かした後だけ） |

候補選択中:

| 操作 | 既定のキー | 内容 |
|------|-----------|------|
| `candidate.next` | space, Tab, Down | 次の候補 |
| `candidate.prev` | Shift+space, Shift+Tab, Up | 前の候補 |
| `candidate.next_page` | Right | 次のページ |
| `candidate.prev_page` | Left | 前のページ |
| `candidate.expand_segment` | Shift+Right | 文節の区切りを右へ伸ばす |
| `candidate.shrink_segment` | Shift+Left | 文節の区切りを左へ縮める |
| `candidate.commit` | Return | 選んでいる候補で確定する |
| `candidate.cancel` | Escape | 入力中に戻る（文節の区切りを調整中なら、調整を取りやめる） |
| `candidate.back` | BackSpace, Control+h | 入力中に戻る |

どちらの場面でも（直接変換）:

| 操作 | 既定のキー | 内容 |
|------|-----------|------|
| `convert_to.hiragana` | F6, Control+u | ひらがなにする |
| `convert_to.katakana_full` | F7, Control+i | 全角カタカナにする |
| `convert_to.katakana_half` | F8, Control+o | 半角カタカナにする |
| `convert_to.alphanumeric_full` | F9, Control+p | 全角英数にする |
| `convert_to.alphanumeric_half` | F10, Control+t | 半角英数にする |

キーはまず今の場面の操作から探し、無ければ直接変換の操作から探します。同じキーを二つの操作に割り当てると、表で上にあるほうが効きます。

この一覧の正本は `hazkey-frontend-core/src/keybindings.cpp` の `actionTable()` です。食い違っていたらそちらが正しいので、この文書を直してください。

## 変えられないキー

- 候補の番号選択: 候補選択中の `1`〜`9`、`0`、入力中・候補選択中の `Alt+1`〜`Alt+9`
- 未確定の文字列が無いときの `space`（空白）と `Shift+space`（半角空白）
- Shift の単押しによる英数入力の切り替え（サーバー側で処理しています）

未確定の文字列がある間は、割り当ての無いキーは何もせずに飲み込まれます。Ctrl を含むキーは文字として入力されません。
