= etcd一巡り

== etcdとは

== etcdを起動してみよう

今回はDockerを利用してetcdを起動するので、まずはDockerがインストールされていることを確認しましょう。

//cmd{
$ docker -v
Docker version 18.06.1-ce, build e68fc7a
//}

次にetcdを起動します。

//cmd{
$ docker run -p 2379:2379 --name etcd \
    -v etcd-data:/var/lib/etcd \
    quay.io/cybozu/etcd:3.3 \
      --listen-client-urls http://0.0.0.0:2379 \
      --advertise-client-urls http://0.0.0.0:2379
//}

Dockerに以下の起動オプションを指定します。

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

//footnote[insecure][安全ではないのでおすすめしないというメッセージが表示されています。安全な接続方法については後ほど解説します。]

//terminal{
2019-03-03 03:53:34.908093 I | etcdmain: etcd Version: 3.3.12
2019-03-03 03:53:34.908213 I | etcdmain: Git SHA: GitNotFound
2019-03-03 03:53:34.908217 I | etcdmain: Go Version: go1.11.4
2019-03-03 03:53:34.908223 I | etcdmain: Go OS/Arch: linux/amd64
  ・・中略・・
2019-03-03 03:53:36.625034 I | etcdserver: published {Name:default ClientURLs:[http://0.0.0.0:2379]} to cluster cdf818194e3a8c32
2019-03-03 03:53:36.625087 I | embed: ready to serve client requests
2019-03-03 03:53:36.625491 N | embed: serving insecure client requests on [::]:2379, this is strongly discouraged!
//}


次にetcdctlを用意します。etcdctlはetcdとやり取りするためのコマンドラインツールです@<fn>{etcdcurl}。
以下のコマンドを実行するとカレントディレクトリにetcdctlがコピーされます。

//footnote[etcdcurl][etcdのAPIはetcdctlを利用せずcurlなどでアクセスすることも可能です。]

//cmd{
$ docker run --rm -u root:root \
    --entrypoint /usr/local/etcd/install-tools \
    --mount type=bind,src=$(pwd),target=/host \
    quay.io/cybozu/etcd:3.3
//}

ではさっそくetcdctlを利用して、etcdの健康状態をチェックしてみましょう。

//cmd{
$ ETCDCTL_API=3 ./etcdctl endpoint health
127.0.0.1:2379 is healthy: successfully committed proposal: took = 915.187µs
//}

現在のetcdはAPIのバージョンとしてv2とv3をサポートしており、etcdctlはデフォルトでAPI v2を利用するようになっています@<fn>{etcdv3}。
API v3を利用するには環境変数@<code>{ETCDCTL_API=3}を指定する必要があります。

//footnote[etcdv3][etcd 3.4から、デフォルトでAPI v3が利用されるようになります。]

== etcdにデータを読み書きしてみよう

* etcdのキーの話
