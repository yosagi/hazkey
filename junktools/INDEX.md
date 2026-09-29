# junktools

再利用可能なユーティリティスクリプト。

- install-deps.sh: hazkey のビルドに必要なシステムパッケージを apt でインストールする（sudo 実行）
- hazkey-deb-install.sh: GitHub Actions の最新成功 run から自ホスト向け deb を取得してインストールする（配布先PCで実行。gh 認証必須、26.04 は noble を流用）
- build-deb-local.sh: clone 直後のリポジトリからローカルで deb をビルドする（install-deps.sh 実行済み前提。出力は tmp/ 配下）
- gen-deb-copyright.sh: deb に入れる著作権・ライセンス表示（/usr/share/doc/fcitx5-hazkey/copyright）を、各同梱物のライセンスファイルから生成する（CI とローカルビルドから呼ばれる。配布元に本文が無いものは deb-copyright/ に置く）
