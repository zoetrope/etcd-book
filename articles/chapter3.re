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

//listnum[compose][docker-compose.yml]{
#@mapfile(../code/chapter3/docker-compose.yml)
version: '3'
services:
  etcd1:
    container_name: etcd1
    image: quay.io/coreos/etcd:v3.3.12
    ports:
      - 2379
      - 2380
    volumes:
      - etcd1-data:/etcd-data
    entrypoint:
      - /usr/local/bin/etcd
      - --data-dir=/etcd-data
      - --name=etcd1
      - --initial-advertise-peer-urls=http://etcd1:2380
      - --listen-peer-urls=http://0.0.0.0:2380
      - --advertise-client-urls=http://etcd1:2379
      - --listen-client-urls=http://0.0.0.0:2379
      - "--initial-cluster=etcd1=http://etcd1:2380,\
           etcd2=http://etcd2:2380,etcd3=http://etcd3:2380"
  etcd2:
    container_name: etcd2
    image: quay.io/coreos/etcd:v3.3.12
    ports:
      - 2379
      - 2380
    volumes:
      - etcd2-data:/etcd-data
    entrypoint:
      - /usr/local/bin/etcd
      - --data-dir=/etcd-data
      - --name=etcd2
      - --initial-advertise-peer-urls=http://etcd2:2380
      - --listen-peer-urls=http://0.0.0.0:2380
      - --advertise-client-urls=http://etcd2:2379
      - --listen-client-urls=http://0.0.0.0:2379
      - "--initial-cluster=etcd1=http://etcd1:2380,\
          etcd2=http://etcd2:2380,etcd3=http://etcd3:2380"
  etcd3:
    container_name: etcd3
    image: quay.io/coreos/etcd:v3.3.12
    ports:
      - 2379
      - 2380
    volumes:
      - etcd3-data:/etcd-data
    entrypoint:
      - /usr/local/bin/etcd
      - --data-dir=/etcd-data
      - --name=etcd3
      - --initial-advertise-peer-urls=http://etcd3:2380
      - --listen-peer-urls=http://0.0.0.0:2380
      - --advertise-client-urls=http://etcd3:2379
      - --listen-client-urls=http://0.0.0.0:2379
      - "--initial-cluster=etcd1=http://etcd1:2380,\
          etcd2=http://etcd2:2380,etcd3=http://etcd3:2380"
volumes:
  etcd1-data:
  etcd2-data:
  etcd3-data:
#@end
//}


//terminal{
$ alias etcdctl='docker exec -e "ETCDCTL_API=3" etcd1 etcdctl --endpoints=http://etcd1:2379,http://etcd2:2379,http://etcd3:2379'
//}

== 証明書

== ユーザー

== スナップショット

== コンパクション

== アップグレード

== モニタリング

== その他
 * cors
 * discover
 * pprof
 * proxy
