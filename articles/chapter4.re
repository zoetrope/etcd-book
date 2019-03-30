= etcdの運用


== クラスタの構築

//list[compose][docker-compose.yml]{
#@mapfile(../code/chapter4/cluster/docker-compose.yml)
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
      - --initial-cluster-state=new
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
      - --initial-cluster-state=new
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
      - --initial-cluster-state=new
volumes:
  etcd1-data:
  etcd2-data:
  etcd3-data:
#@end
//}

//terminal{
$ docker inspect --format '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' etcd1
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

まずCSR(証明書署名要求)の設定ファイルを作成します。
このファイルには、公開鍵の暗号化方式や組織の所在地などを含めます。

//list[ca-csr][ca-csr.json]{
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

この設定ファイルを使用して自己署名ルートCA証明書と秘密鍵を生成します。

//terminal{
#@maprange(../code/chapter4/tls/gen-certs.sh,ca)
cfssl gencert -initca ca-csr.json | cfssljson -bare certs/ca
#@end
//}

次にCAの設定を記述します。

//list[ca-config][ca-config.json]{
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

//list[server][server.json]{
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

//terminal{
#@maprange(../code/chapter4/tls/gen-certs.sh,server)
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=server server.json | cfssljson -bare certs/server
#@end
//}

//list[etcd1][etcd1.json]{
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

//terminal{
#@maprange(../code/chapter4/tls/gen-certs.sh,peer)
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=peer etcd1.json | cfssljson -bare certs/etcd1
#@end
//}

//list[client][client.json]{
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

//terminal{
#@maprange(../code/chapter4/tls/gen-certs.sh,client)
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=client client.json | cfssljson -bare certs/client
#@end
//}

//list[docker-compose][docker-compose.yml]{
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
      - 2379
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
      - --initial-cluster-state=new
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
$ ETCDCTL_API=3 etcdctl --endpoints=https://172.30.0.11:2379,https://172.30.0.12:2379,https://172.30.0.13:2379 --key=./certs/client-key.pem --cert=./certs/client.pem --insecure-skip-tls-verify=true member list 
1f6fd35e3327767a, started, etcd1, https://etcd1:2380, https://etcd1:2379
2a6277f8728ef760, started, etcd3, https://etcd3:2380, https://etcd3:2379
4acd0a1e9189cd7a, started, etcd2, https://etcd2:2380, https://etcd2:2379
//}

== メンバーの追加・削除

//terminal{
$ etcdctl member add etcd4 --peer-urls=http://etcd4:2380
Member 97754dc05c5b1f8d added to cluster 8f2a2ec5c0087dcb

ETCD_NAME="etcd4"
ETCD_INITIAL_CLUSTER="etcd4=http://etcd4:2380,etcd1=http://etcd1:2380,etcd3=http://etcd3:2380,etcd2=http://etcd2:2380"
ETCD_INITIAL_ADVERTISE_PEER_URLS="http://etcd4:2380"
ETCD_INITIAL_CLUSTER_STATE="existing"
//}


//terminal{
$ docker-compose up etcd1 etcd2 etcd3
//}

//terminal{
$ docker-compose up etcd4
//}

== ユーザー

== スナップショット

== コンパクション

== アップグレード


== discover / naming
 go-grpc向けの便利機能。
 gRPCサーバーの接続先が増えた時に自動的に対応。ロードバランスもしてくれる。
 KubernetesとかNaming Service使ってるなら不要。

//list[?][helloworld.proto]{
#@mapfile(../code/chapter4/naming/pb/helloworld.proto)
syntax = "proto3";

package main;

service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {}
}

message HelloRequest {
  string name = 1;
}

message HelloReply {
  string message = 1;
}
#@end
//}

//list[?][server.go]{
#@mapfile(../code/chapter4/naming/server/server.go)
//go:generate protoc -I ../pb --go_out=plugins=grpc:./ ../pb/helloworld.proto
package main

import (
    "context"
    "log"
    "net"
    "time"

    "github.com/coreos/etcd/clientv3"
    "github.com/coreos/etcd/clientv3/naming"
    "google.golang.org/grpc"
    gn "google.golang.org/grpc/naming"
)

type greeter struct{}

func (s *greeter) SayHello(ctx context.Context, in *HelloRequest) (*HelloReply, error) {
    log.Printf("Received: %v", in.Name)
    return &HelloReply{Message: "Hello " + in.Name}, nil
}

func main() {
    client, err := clientv3.New(clientv3.Config{
        Endpoints:   []string{"http://localhost:2379"},
        DialTimeout: 3 * time.Second,
    })
    if err != nil {
        log.Fatal(err)
    }
    defer client.Close()

    listener, err := net.Listen("tcp", "127.0.0.1:0")
    if err != nil {
        log.Fatal(err)
    }

    resolver := &naming.GRPCResolver{Client: client}
    err = resolver.Update(context.TODO(), "/chapter4/greeter",
        gn.Update{Op: gn.Add, Addr: listener.Addr().String()})
    if err != nil {
        log.Fatal(err)
    }

    server := grpc.NewServer()
    RegisterGreeterServer(server, &greeter{})
    if err := server.Serve(listener); err != nil {
        log.Fatal(err)
    }
}
#@end
//}

etcdにキーが残り続ける。
終了時に削除するか、Leaseを設定しておいてサーバーが落ちて少し時間が経ったら消えるようにしておくのがよい。

//list[?][client.go]{
#@mapfile(../code/chapter4/naming/client/client.go)
//go:generate protoc -I ../pb --go_out=plugins=grpc:./ ../pb/helloworld.proto
package main

import (
    "context"
    "log"
    "time"

    "github.com/coreos/etcd/clientv3"
    "github.com/coreos/etcd/clientv3/naming"
    "google.golang.org/grpc"
)

func main() {
    client, err := clientv3.New(clientv3.Config{
        Endpoints:   []string{"http://localhost:2379"},
        DialTimeout: 3 * time.Second,
    })
    if err != nil {
        log.Fatal(err)
    }
    defer client.Close()

    resolver := &naming.GRPCResolver{Client: client}
    balancer := grpc.RoundRobin(resolver)

    conn, err := grpc.Dial("/chapter4/greeter", grpc.WithBalancer(balancer),
        grpc.WithInsecure(), grpc.WithBlock())
    if err != nil {
        log.Fatal(err)
    }
    defer conn.Close()
    c := NewGreeterClient(conn)

    r, err := c.SayHello(context.TODO(), &HelloRequest{Name: "World!"})
    if err != nil {
        log.Fatal(err)
    }
    log.Printf("Greeting: %s", r.Message)
}
#@end
//}

grpc.WithBlock()がないとエラーになるので注意。





== その他
 * cors

 * pprof
 * proxy
 * defrag
 * mirror
 * balancer (etcd v3.4)
 * leasing
 * ordering

 * gRPC Proxy(Scalable Watch API)
 https://coreos.com/etcd/docs/latest/op-guide/grpc_proxy.html

 * grpc-gateway
 gRPCをJSONのREST形式に変換してくれるだけ。etcdをcurlとかで呼べるのはこれのおかげ。

 * etcd gateway
 接続先が変わった時にクライアントを更新するのは面倒。ゲートウェイを使えば、ゲートウェイだけ更新すればよい。
 ローカルホストを指しておけばいいようにしておく。
 各クライアントの動いてるところで動かす必要がある。
 DNSを使えば自動的に更新することも可能。
 https://coreos.com/etcd/docs/latest/op-guide/gateway.html
