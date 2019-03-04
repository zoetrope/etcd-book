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
$ docker run --name etcd \
    quay.io/coreos/etcd:v3.3.12
//}

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

次にetcdctlを使ってみましょう。etcdctlはetcdとやり取りするためのコマンドラインツールです@<fn>{etcdcurl}。

//footnote[etcdcurl][etcdのAPIはetcdctlを利用せずcurlなどでアクセスすることも可能です。]

//cmd{
$ docker exec etcd etcdctl endpoint health
127.0.0.1:2379 is healthy: successfully committed proposal: took = 1.154489ms
//}

現在のetcdはAPIのバージョンとしてv2とv3をサポートしており、etcdctlはデフォルトでAPI v2を利用するようになっています@<fn>{etcdv3}。
API v3を利用するには環境変数@<code>{ETCDCTL_API=3}を指定する必要があります。

//footnote[etcdv3][etcd 3.4から、デフォルトでAPI v3が利用されるようになります。]

毎回長いコマンドを打ち込むのは面倒なので、以下のようなエイリアスを用意しておくと便利でしょう。

//cmd{
$ alias etcdctl='docker exec -e "ETCDCTL_API=3" etcd etcdctl'
//}

== etcdにデータを読み書きしてみよう

=== etcdのキーの話

=== VersionとRevisionの話

etcdctlで値を取得する時に@<code>{--write-out=json}を指定すると詳細な情報を表示することができます@<fn>{base64}。

//footnote[base64][このときのキーとバリューの値はBASE64形式でエンコードされています。]

//terminal{
$ etcdctl get foo --write-out=json | jq
{
  "header": {
    "cluster_id": 14841639068965180000,
    "member_id": 10276657743932975000,
    "revision": 2,
    "raft_term": 2
  },
  "kvs": [
    {
      "key": "Zm9v",
      "create_revision": 2,
      "mod_revision": 2,
      "version": 1,
      "value": "YmFy"
    }
  ],
  "count": 1
}
//}

: revision
    etcdのリビジョン番号。etcdに何らかの変更(キーの追加、変更、削除)が加えられると値が1増える。
: create_revision
    このキーが作成されたときのリビジョン番号。
: mod_revision
    このキーの内容が変更されたときのリビジョン番号。
: version
    このキーのバージョン。このキーに変更が加えられると値が1増える。



