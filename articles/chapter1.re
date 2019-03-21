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




=== キースペース

etcd v2では、キーをファイルシステムのように/で区切って階層的に管理していました。
etcd v3では、キーを単一のバイト配列としてフラットなキースペースで管理するように変わりました。
例えばキーの名前に/this/is/a/keyだったとしても、キーはただのバイト配列として扱われます。

etcd v2のときは、特定の階層に対してアクセス権を設定したり、値をウォッチすることができました。
etcd v3では、特定のプレフィックスで始まるキーに対してアクセス権を設定したり、値をウォッチすることができます。

namepaceや、アクセス権の

しかし、慣例的に/で区切って管理することが多くなっています。


例えばKubernetesでは/registry/から始まるキーを使います。(歴史的経緯で一部minionから始まるキーもある)

 * /registry/pods/default/mypod
 * /registry/services/kube-system/default

=== バリュー

値にはJSONやProtocol Buffer
最大1MB
