# junktools

再利用可能なユーティリティスクリプト。

- install-deps.sh: hazkey のビルドに必要なシステムパッケージを apt でインストールする（sudo 実行）
- hazkey-deb-install.sh: GitHub Actions の最新成功 run から自ホスト向け deb を取得してインストールする（配布先PCで実行。gh 認証必須。パッケージを引数で選べ、hazkey-server は常に入る）
- build-deb-local.sh: clone 直後のリポジトリからローカルで deb 一式をビルドする（install-deps.sh 実行済み前提。出力は tmp/debs/）
- make-debs.sh: パッケージごとに DESTDIR でインストールしたツリーから deb を作る（strip、著作権表示、依存の検出、control。CI とローカルビルドから呼ばれる）
- gen-deb-copyright.sh: deb に入れる著作権・ライセンス表示（/usr/share/doc/<package>/copyright）を、各同梱物のライセンスファイルから生成する（make-debs.sh から呼ばれる。配布元に本文が無いものは deb-copyright/ に置く）
