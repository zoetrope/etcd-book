= etcdの運用

== etcdの起動

//cmd{
$ docker run \
    -p 2379:2379 \
    -p 2380:2380 \
    --volume=etcd-data:/etcd-data \
    --name etcd quay.io/coreos/etcd:v3.3.12 \
    /usr/local/bin/etcd \
      --data-dir=/etcd-data --name node1 \
      --initial-advertise-peer-urls http://${NODE1}:2380 --listen-peer-urls http://${NODE1}:2380 \
      --advertise-client-urls http://${NODE1}:2379 --listen-client-urls http://${NODE1}:2379 \
      --initial-cluster node1=http://${NODE1}:2380
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


== クラスタの構築

== 証明書

== ユーザー

== スナップショット

== コンパクション

== アップグレード

== その他
 * metrics
 * cors
 * discover
 * pprof
 * proxy
