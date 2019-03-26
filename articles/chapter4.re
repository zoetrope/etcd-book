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
#@mapfile(../code/chapter4/docker-compose.yml)
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

ここではCloudFlare社のcfsslというツールを利用して証明書の発行をおこないます。

まずはcfsslとcfssljsonのセットアップをおこないます。

//terminal{
$ go get -u github.com/cloudflare/cfssl/cmd/cfssl
$ go get -u github.com/cloudflare/cfssl/cmd/cfssljson
//}

まずはCA用のCSR(証明書署名要求)を生成するための設定ファイルを作成します。
このファイルには、公開鍵の暗号化方式や組織の所在地などを含めます。

//listnum[ca-csr][ca-csr.json]{
#@mapfile(../code/chapter4/tls/ca-csr.json)
{
    "CN": "test.local",
    "key": {
        "algo": "rsa",
        "size": 2048
    },
    "names": [
        {
            "C": "JP",
            "L": "Tokyo",
            "ST": "Chuo-ku"
        }
    ]
}
#@end
//}

この設定ファイルを使用してCA

//list[][]{
#@maprange(../code/chapter4/tls/gen-certs.sh,ca)
cfssl gencert -initca ca-csr.json | cfssljson -bare certs/ca
#@end
//}

次にCAの設定を記述します。

//listnum[ca-config][ca-config.json]{
#@mapfile(../code/chapter4/tls/ca-config.json)
{
    "signing": {
        "default": {
            "expiry": "43800h"
        },
        "profiles": {
            "server": {
                "expiry": "43800h",
                "usages": [
                    "signing",
                    "key encipherment",
                    "server auth"
                ]
            },
            "client": {
                "expiry": "43800h",
                "usages": [
                    "signing",
                    "key encipherment",
                    "client auth"
                ]
            },
            "peer": {
                "expiry": "43800h",
                "usages": [
                    "signing",
                    "key encipherment",
                    "server auth",
                    "client auth"
                ]
            }
        }
    }
}
#@end
//}

//listnum[server][server.json]{
#@mapfile(../code/chapter4/tls/server.json)
{
    "CN": "etcd",
    "hosts": [
        "172.30.0.11",
        "etcd1",
        "172.30.0.12",
        "etcd2",
        "172.30.0.13",
        "etcd3"
    ],
    "key": {
        "algo": "rsa",
        "size": 2048
    },
    "names": [
        {
            "C": "JP",
            "L": "Tokyo",
            "ST": "Chuo-ku"
        }
    ]
}
#@end
//}

//list[][]{
#@maprange(../code/chapter4/tls/gen-certs.sh,server)
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=server server.json | cfssljson -bare certs/server
#@end
//}

//listnum[etcd1][etcd1.json]{
#@mapfile(../code/chapter4/tls/etcd1.json)
{
    "CN": "etcd1",
    "hosts": [
        "172.30.0.11",
        "etcd1"
    ],
    "key": {
        "algo": "rsa",
        "size": 2048
    },
    "names": [
        {
            "C": "JP",
            "L": "Tokyo",
            "ST": "Chuo-ku"
        }
    ]
}
#@end
//}

//list[][]{
#@maprange(../code/chapter4/tls/gen-certs.sh,peer)
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=peer etcd1.json | cfssljson -bare certs/etcd1
#@end
//}

//listnum[client][client.json]{
#@mapfile(../code/chapter4/tls/client.json)
{
    "CN": "client",
    "hosts": [""],
    "key": {
        "algo": "rsa",
        "size": 2048
    },
    "names": [
        {
            "C": "JP",
            "L": "Tokyo",
            "ST": "Chuo-ku"
        }
    ]
}
#@end
//}

//list[][]{
#@maprange(../code/chapter4/tls/gen-certs.sh,client)
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=client client.json | cfssljson -bare certs/client
#@end
//}

//listnum[docker-compose][docker-compose.yml]{
#@maprange(../code/chapter4/tls/docker-compose.yml,etcd1)
version: '3'
services:
  etcd1:
    container_name: etcd1
    image: quay.io/coreos/etcd:v3.3.12
    networks:
      app_net:
        ipv4_address: 172.30.0.11
    ports:
      - 23791:2379
      - 2380
    volumes:
      - etcd1-data:/etcd-data
      - ./certs/:/certs/
    entrypoint:
      - /usr/local/bin/etcd
      - --data-dir=/etcd-data
      - --name=etcd1
      - --initial-advertise-peer-urls=https://etcd1:2380
      - --listen-peer-urls=https://0.0.0.0:2380
      - --advertise-client-urls=https://etcd1:2379
      - --listen-client-urls=https://0.0.0.0:2379
      - "--initial-cluster=etcd1=https://etcd1:2380,\
           etcd2=https://etcd2:2380,etcd3=https://etcd3:2380"
      - --cert-file=/certs/server.pem
      - --key-file=/certs/server-key.pem
      - --client-cert-auth=true
      - --trusted-ca-file=/certs/ca.pem
      - --peer-cert-file=/certs/etcd1.pem
      - --peer-key-file=/certs/etcd1-key.pem
      - --peer-client-cert-auth=true
      - --peer-trusted-ca-file=/certs/ca.pem
#@end
  # etcd2, etcd3はほとんど同じなので省略
#@maprange(../code/chapter4/tls/docker-compose.yml,networks)
networks:
  app_net:
    driver: bridge
    ipam:
      driver: default
      config:
        - subnet: 172.30.0.0/24
volumes:
  etcd1-data:
  etcd2-data:
  etcd3-data:
#@end
//}


//terminal{
$ ETCDCTL_API=3 etcdctl --endpoints=https://localhost:23791,https://localhost:23792,https://localhost:23793 --key=./certs/client-key.pem --cert=./certs/client.pem --insecure-skip-tls-verify=true member list 
1f6fd35e3327767a, started, etcd1, https://etcd1:2380, https://etcd1:2379
2a6277f8728ef760, started, etcd3, https://etcd3:2380, https://etcd3:2379
4acd0a1e9189cd7a, started, etcd2, https://etcd2:2380, https://etcd2:2379
//}


== Namespace

== ユーザー

== スナップショット

== コンパクション

== メンバーの追加・削除

メンバーが追加できるようになるまで少し時間がかかるので注意。

== アップグレード

== モニタリング

== その他
 * cors
 * discover / naming
 * pprof
 * proxy
 * defrag
 * mirror
 * balancer (etcd v3.4)
 * leasing
 * ordering
 * gRPC Proxy(Scalable Watch API)
