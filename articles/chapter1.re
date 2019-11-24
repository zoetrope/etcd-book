= etcd一巡り

== etcdを起動してみよう

今回はDockerを利用してetcdを起動するので、まずはDockerがインストールされていることを確認しましょう。

//cmd{
$ docker -v     
Docker version 19.03.5, build 633a0ea838
//}

次にetcdを起動します。

//cmd{
$ docker run --name etcd \
    -p 2379:2379 \
    --volume=etcd-data:/etcd-data \
    --name etcd gcr.io/etcd-development/etcd:v3.3.17 \
    /usr/local/bin/etcd \
      --name=etcd-1 \
      --data-dir=/etcd-data \
      --advertise-client-urls http://0.0.0.0:2379 \
      --listen-client-urls http://0.0.0.0:2379
//}

Dockerに以下のオプションを指定します。

: -p 2379:2379
    コンテナ外からetcdのAPIを呼び出せるように2379番ポートをホストにバインドしています。
: -v etcd-data:/var/lib/etcd
    etcd-dataというボリュームを作成し、コンテナの@<code>{/var/lib/etcd}にバインドしています。これによりコンテナを終了してもetcdのデータは消えません。

etcdに以下の起動オプションを指定します。

: --listen-client-urls
    etcdがクライアントからのリクエストを受け付けるURLを指定します。
: --advertise-client-urls
    クライアント用のURLをクラスタの他のメンバーに伝えるために指定します。
    現在はクラスタを組んでいませんが、@<code>{--listen-client-urls}を指定した場合は必ずこのオプションも指定する必要があります。

以下のようなログが出力されればetcdの起動は成功です@<fn>{insecure}。

//footnote[insecure][serving insecure client requests on [::\]:2379, this is strongly discouraged!というメッセージが表示されていますがここでは無視します。安全な接続方法については後ほど解説します。]

//terminal{
2019-11-24 05:29:22.996364 I | etcdmain: etcd Version: 3.3.17
2019-11-24 05:29:22.996423 I | etcdmain: Git SHA: 6d8052314
2019-11-24 05:29:22.996427 I | etcdmain: Go Version: go1.12.9
2019-11-24 05:29:22.996430 I | etcdmain: Go OS/Arch: linux/amd64
  ・・中略・・
2019-11-24 05:29:24.603553 I | etcdserver: published {Name:etcd-1 ClientURLs:[http://0.0.0.0:2379]} to cluster cdf818194e3a8c32
2019-11-24 05:29:24.603602 I | embed: ready to serve client requests
2019-11-24 05:29:24.603989 N | embed: serving insecure client requests on [::]:2379, this is strongly discouraged!
//}

次にetcdctlを使います。etcdctlはetcdとやり取りするためのコマンドラインツールです。
etcdctlもDockerコンテナの中に含まれています。
先ほど起動したコンテナに入って、etcdctlを実行してみましょう。

//terminal{
$ docker exec -e "ETCDCTL_API=3" etcd etcdctl
NAME:
        etcdctl - A simple command line client for etcd3.

USAGE:
        etcdctl

VERSION:
        3.3.17

API VERSION:
        3.3


COMMANDS:
        以下省略
//}

現在のetcdはAPIのバージョンとしてv2とv3をサポートしており、etcdctlはデフォルトでAPI v2を利用するようになっています。
API v3を利用するには環境変数@<code>{ETCDCTL_API=3}を指定する必要があります@<fn>{etcdv3}。

//footnote[etcdv3][etcd 3.4から、デフォルトでAPI v3が利用されるようになります。]

毎回長いコマンドを打ち込むのは面倒なので、以下のようなエイリアスを用意しておくと便利です。

//terminal{
$ alias etcdctl='docker exec -e "ETCDCTL_API=3" etcd etcdctl'
//}

== etcdにデータを読み書きしてみよう

etcdctlを利用してetcdにデータを書き込んでみます。
ここでは@<code>{/chapter1/hello}というキーに@<code>{Hello, World!}といバリューを書き込んでいます。
書き込みに成功するとOKと表示されます。

//terminal{
$ etcdctl put /chapter1/hello 'Hello, World!'
OK
//}

次にキーを指定して値を読み込んでみましょう。
@<code>{/chapter1/hello}というキーを指定してgetコマンドを実行すると、先ほど書き込んだバリューが表示されます。

//terminal{
$ etcdctl get /chapter1/hello            
/chapter1/hello
Hello, World!
//}

=== キー

先程の例でetcdにデータを書き込むときに@<code>{/chapter1/hello}というキーを利用しました。
このキーの名前はどのように決めればいいのでしょうか？
etcd v3ではキーは内部的にはバイト配列として扱われており、どのような文字列でも利用することができます。
日本語を利用することも可能です。

//note[キースペースの変更]{
etcd v2とv3ではキーの管理方法が大きく変わりました。
etcd v2ではキーをファイルシステムのように/で区切って階層的に管理していました。
etcd v3からはキーを単一のバイト配列としてフラットなキースペースで管理するようになりました。
//}

etcdには、MySQLやPostgreSQLにおけるデータベースやテーブルのような概念はありません。
そのため複数のアプリケーションで1つのetcdクラスタを共有する場合は、アプリケーションごとにキーのプレフィックスを決めておくことが一般的です。
例えばアプリケーション1は@<code>{/app1/}から始まるキー、アプリケーション2は@<code>{/app2/}から始まるキーを使うというような決まりにします。
etcdではキーのプレフィックスに対してアクセス権を設定することができるので、@<code>{/app1/}から始まるキーはアプリケーション1の実行ユーザーのみが
読み書き可能な設定にしておきます。

具体的にKubernetesの例を見てみましょう。
Kubernetesが動いている環境で、etcdctlを利用してキーの一覧を取得してみます。

//terminal{
$ etcdctl get / --prefix=true --keys-only
/registry/pods/default/mypod
/registry/services/kube-system/nginx
  以下省略
//}

このようにKubernetesは@<code>{/registry/}から始まるキーを利用しています。
その後ろにリソースのタイプ(podやservice)、ネームスペース(defaultやkube-system)、リソースの名前(mypodやnginx)を@<code>{/}でつないでキーにしています。

=== バリュー

キーは@<code>{/}で区切った文字列を利用するのが一般的だと解説しました。
ではバリューはどのような形式にするのがよいのでしょうか？
バリューも内部的にはバイト配列として扱われています。
1つのバリューのサイズは最大1MBまで格納することができます。

## etcd v3.4では1.5MiBに？これはリクエストサイズなのでバリューサイズではない？
## https://github.com/etcd-io/etcd/blob/release-3.3/Documentation/dev-guide/limit.md

バリューは単純な文字列として格納してもよいですし、構造的なデータを扱いたいのであれば
JSONやProtocol Bufferなどでエンコードした形式で格納してもよいでしょう。

具体的にKubernetesの例を見てみましょう。
先ほどと同じように、Kubernetesが動いている環境でetcdctlを利用してバリューを取得してみます。

//terminal{
$ etcdctl get /registry/pods/default/mypod
//}

このコマンドを実行すると、バイナリ形式のデータが出力されます。

Kubernetesではv1.6以降はProtocol Buffer形式でデータを格納しているため、そのままでは読むことができません。
Auger@<fn>{auger}などのツールを利用すればデコードして表示することも可能です。

//footnote[auger][https://github.com/jpbetz/auger]

なお、etcdのデータファイルにはキーバリューの内容がそのまま平文で保存されています。
秘密情報などを管理したい場合はバリューを暗号化して格納する必要があります。
