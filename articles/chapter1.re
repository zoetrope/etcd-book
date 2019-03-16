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

=== RevisionとVersion

MVCC (MultiVersion Concurrency Control)

etcdにおける重要な要素の一つとしてRevisionがあります。

etcdctlで値を取得する時に@<code>{--write-out=json}を指定すると詳細な情報を表示することができます@<fn>{base64}。
/key1に値を書き込み、詳細な情報を取得してみましょう@<fn>{jq}。

//footnote[base64][このときのキーとバリューの値はBASE64形式でエンコードされています。]
//footnote[jq][JSONの整形のためにjqを利用します。@<href>{https://stedolan.github.io/jq/}]

//terminal{
$ etcdctl put /key1 value1
OK
$ etcdctl get /key1 --write-out=json | jq
{
  "header": {
    "cluster_id": 14841639068965180000,
    "member_id": 10276657743932975000,
    "revision": 2,
    "raft_term": 2
  },
  "kvs": [
    {
      "key": "L2tleTE=",
      "create_revision": 2,
      "mod_revision": 2,
      "version": 1,
      "value": "dmFsdWUx"
    }
  ],
  "count": 1
}
//}

取得した情報の中に、revisionやversionなどのフィールドが見つかると思います。

: revision
    etcdのリビジョン番号。クラスタ全体で一つの値が利用される。etcdに何らかの変更(キーの追加、変更、削除)が加えられると値が1増える。
: create_revision
    このキーが作成されたときのリビジョン番号。
: mod_revision
    このキーの内容が変更されたときのリビジョン番号。
: version
    このキーのバージョン。このキーに変更が加えられると値が1増える。

次に/key1の値を更新してみましょう。revision, mod_revision, versionの値が1つ増え、create_revisionがそのままの値になっています。

//terminal{
$ etcdctl put /key1 value2
OK
$ etcdctl get /key1 --write-out=json \
    | jq '(.header | {revision}), (.kvs[] | { create_revision, mod_revision, version})'
{
  "revision": 3
}
{
  "create_revision": 2,
  "mod_revision": 3,
  "version": 2
}
//}

次に別のキー/key2に値を書き込んでみましょう。このときの/key1の情報を見ると、revisionは増えているものの
create_revision, mod_revision, versionは値が変化していません。

//terminal{
$ etcdctl put /key2 value3
OK
$ etcdctl get /key1 --write-out=json \
    | jq '(.header | {revision}), (.kvs[] | { create_revision, mod_revision, version})'
{
  "revision": 4
}
{
  "create_revision": 2,
  "mod_revision": 3,
  "version": 2
}
//}

一方、/key2の情報を見ると、revisionは/key1と同じ値になっていますが、versionは新たに1になっていることがわかります。

//terminal{
$ etcdctl get /key2 --write-out=json \
    | jq '(.header | {revision}), (.kvs[] | { create_revision, mod_revision, version})'
{
  "revision": 4
}
{
  "create_revision": 4,
  "mod_revision": 4,
  "version": 1
}
//}

ここで説明したrevisionは、後ほどのトランザクションやウォッチの
