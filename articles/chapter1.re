= etcd一巡り

== etcdを起動してみよう

今回はDockerを利用してetcdを起動するので、まずはDockerがインストールされていることを確認しましょう。

//cmd{
$ docker -v
Docker version 18.06.1-ce, build e68fc7a
//}

次にetcdを起動します。

//cmd{
$ docker run --name etcd \
    -p 2379:2379 \
    --volume=etcd-data:/etcd-data \
    --name etcd quay.io/coreos/etcd:v3.3.12 \
    /usr/local/bin/etcd \
      --name=etcd-1 \
      --data-dir=/etcd-data \
      --advertise-client-urls http://0.0.0.0:2379 \
      --listen-client-urls http://0.0.0.0:2379
//}

: -p 2379:2379
    コンテナ外からetcdのAPIを呼び出せるように2379番ポートをホストにバインドしています。
: -v etcd-data:/var/lib/etcd
    etcd-dataというボリュームを作成し、コンテナの@<code>{/var/lib/etcd}にバインドしています。これによりコンテナを終了してもetcdのデータは
消えません。

etcdに以下の起動オプションを指定します。

: --listen-client-urls
    etcdがクライアントからのリクエストを受け付けるURLを指定します。
: --advertise-client-urls
    クライアント用のURLをクラスタの他のメンバーに伝えるために指定します。
    現在はクラスタを組んでいませんが、@<code>{--listen-client-urls}を指定した場合は必ずこのオプションも指定する必要があります。

以下のようなログが出力されればetcdの起動は成功です@<fn>{insecure}。

//footnote[insecure][安全ではないのでおすすめしないというメッセージが表示されています。安全な接続方法については後ほど解説します。]

//terminal{
2019-03-03 03:53:34.908093 I | etcdmain: etcd Version: 3.3.12
2019-03-03 03:53:34.908213 I | etcdmain: Git SHA: GitNotFound
2019-03-03 03:53:34.908217 I | etcdmain: Go Version: go1.10.8
2019-03-03 03:53:34.908223 I | etcdmain: Go OS/Arch: linux/amd64
  ・・中略・・
2019-03-03 03:53:36.625034 I | etcdserver: published {Name:etcd-1 ClientURLs:[http://0.0.0.0:2379]} to cluster cdf818194e3a8c32
2019-03-03 03:53:36.625087 I | embed: ready to serve client requests
2019-03-03 03:53:36.625491 N | embed: serving insecure client requests on [::]:2379, this is strongly discouraged!
//}

次にetcdctlを使ってみましょう。etcdctlはetcdとやり取りするためのコマンドラインツールです。

//terminal{
$ docker exec -e "ETCDCTL_API=3" etcd etcdctl --endpoint=http://127.0.0.1:2379 endpoint health
127.0.0.1:2379 is healthy: successfully committed proposal: took = 1.154489ms
//}

現在のetcdはAPIのバージョンとしてv2とv3をサポートしており、etcdctlはデフォルトでAPI v2を利用するようになっています。
API v3を利用するには環境変数@<code>{ETCDCTL_API=3}を指定する必要があります@<fn>{etcdv3}。

//footnote[etcdv3][etcd 3.4から、デフォルトでAPI v3が利用されるようになります。]

毎回長いコマンドを打ち込むのは面倒なので、以下のようなエイリアスを用意しておくと便利でしょう。

//terminal{
$ alias etcdctl='docker exec -e "ETCDCTL_API=3" etcd etcdctl --endpoint=http://127.0.0.1:2379'
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
@<code>{/chapter1/hello}というキーを指定してgetコマンドを実行すると、先程書き込んだバリューが表示されます。

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
etcd v3からはキーを単一のバイト配列としてフラットなキースペースで管理するように。
//}

etcdには、MySQLやPostgreSQLにあるようなデータベースやテーブルのような概念はありません。
そのため複数のアプリケーションで1つのetcdクラスタを共有する場合は、アプリケーションごとにキーのプレフィックスを決めておくことが一般的です。
例えばアプリケーション1は@<code>{/app1/}から始まるキー、アプリケーション2は@<code>{/app2/}から始まるキーを使うというような決まりにします。
etcdではキーのプレフィックスに対してアクセス権を設定することができるので、@<code>{/app1/}から始まるキーはアプリケーション1の実行ユーザーのみが
読み書き可能な設定にしておきます。

具体的にKubernetesの例を見てみましょう。
以下のようにKubernetesは@<code>{/registry/}から始まるキーを利用しています。
その後ろにリソースのタイプ(podやservice)、ネームスペース(defaultやkube-system)、リソースの名前(mypodやnginx)を@<code>{/}でつないでキーにしています。

//terminal{
$ etcdctl get / --prefix=true --keys-only
/registry/pods/default/mypod
/registry/services/kube-system/nginx
//}

=== バリュー

キーは@<code>{/}で区切った文字列を利用することが一般的です。
ではバリューはどのような形式にするのがよいのでしょうか？
バリューも内部的にはバイト配列として扱われています。
1つのバリューのサイズは最大1MBまで格納することができます。

## etcd v3.4では1.5MiBに？これはリクエストサイズなのでバリューサイズではない？
## https://github.com/etcd-io/etcd/blob/release-3.3/Documentation/dev-guide/limit.md

バリューは単純な文字列として格納してもよいですし、構造的なデータを扱いたいのであれば
JSONやProtocol Bufferなどでエンコードした形式で格納してもよいでしょう。

具体的にKubernetesの例を見てみましょう。
以下のコマンドを実行してバリューを取得すると、バイナリ形式のデータが出力されます。

//terminal{
$ etcdctl get /registry/pods/default/mypod
//}

Kubernetesではv1.6以降はProtocol Buffer形式でデータを格納しているため、そのままでは読むことができません。
Auger@<fn>{auger}などのツールを利用すればデコードして表示することも可能です。

//footnote[auger][https://github.com/jpbetz/auger]

なお、etcdのデータファイルにはキーバリューの内容がそのまま平文で保存されています。
秘密情報などを管理したい場合はバリューを暗号化して格納すべきです。
