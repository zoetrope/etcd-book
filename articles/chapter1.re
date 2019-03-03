= etcdとは

== etcdを触ってみよう

今回はDockerを利用してetcdを起動するので、まずはDockerがインストールされていることを確認しましょう。

//cmd{
$ docker -v
Docker version 18.06.1-ce, build e68fc7a
//}

次にetcdを起動します。

//cmd{
$ docker run --rm -p 2379:2379 --name etcd \
    -v etcd-data:/var/lib/etcd \
    quay.io/cybozu/etcd:3.3 \
      --advertise-client-urls http://0.0.0.0:2379 \
      --listen-client-urls http://0.0.0.0:2379
//}

以下のようなログが出力されていれば、etcdの起動は成功です。

//terminal{
2019-03-02 01:38:31.666227 I | etcdmain: etcd Version: 3.3.9
2019-03-02 01:38:31.666329 I | etcdmain: Git SHA: GitNotFound
2019-03-02 01:38:31.666333 I | etcdmain: Go Version: go1.11
2019-03-02 01:38:31.666336 I | etcdmain: Go OS/Arch: linux/amd64
2019-03-02 01:38:31.666342 I | etcdmain: setting maximum number　 of CPUs to 4, total number of available CPUs is 4
2019-03-02 01:38:31.666382 N | etcdmain: the server is already initialized as member before, starting as etcd member...
2019-03-02 01:38:31.666647 I | embed: listening for peers on http://localhost:2380
2019-03-02 01:38:31.666696 I | embed: listening for client requests on 0.0.0.0:2379
2019-03-02 01:38:31.667079 I | etcdserver: name = default
2019-03-02 01:38:31.667087 I | etcdserver: data dir = /var/lib/etcd
2019-03-02 01:38:31.667092 I | etcdserver: member dir = /var/lib/etcd/member
2019-03-02 01:38:31.667096 I | etcdserver: heartbeat = 100ms
2019-03-02 01:38:31.667099 I | etcdserver: election = 1000ms
2019-03-02 01:38:31.667102 I | etcdserver: snapshot count = 100000
2019-03-02 01:38:31.667111 I | etcdserver: advertise client URLs = http://0.0.0.0:2379
2019-03-02 01:38:31.667293 I | etcdserver: restarting member 8e9e05c52164694d in cluster cdf818194e3a8c32 at commit index 12
2019-03-02 01:38:31.667308 I | raft: 8e9e05c52164694d became follower at term 6
2019-03-02 01:38:31.667315 I | raft: newRaft 8e9e05c52164694d [peers: [], term: 6, commit: 12, applied: 0, lastindex: 12, lastterm: 6]
2019-03-02 01:38:31.678779 W | auth: simple token is not cryptographically signed
2019-03-02 01:38:31.680978 I | etcdserver: starting server... [version: 3.3.9, cluster version: to_be_decided]
2019-03-02 01:38:31.681463 I | etcdserver/membership: added member 8e9e05c52164694d [http://localhost:2380] to cluster cdf818194e3a8c32
2019-03-02 01:38:31.681537 N | etcdserver/membership: set the initial cluster version to 3.3
2019-03-02 01:38:31.681565 I | etcdserver/api: enabled capabilities for version 3.3
2019-03-02 01:38:32.668342 I | raft: 8e9e05c52164694d is starting a new election at term 6
2019-03-02 01:38:32.668396 I | raft: 8e9e05c52164694d became candidate at term 7
2019-03-02 01:38:32.668422 I | raft: 8e9e05c52164694d received MsgVoteResp from 8e9e05c52164694d at term 7
2019-03-02 01:38:32.668445 I | raft: 8e9e05c52164694d became leader at term 7
2019-03-02 01:38:32.668458 I | raft: raft.node: 8e9e05c52164694d elected leader 8e9e05c52164694d at term 7
2019-03-02 01:38:32.668833 I | etcdserver: published {Name:default ClientURLs:[http://0.0.0.0:2379]} to cluster cdf818194e3a8c32
2019-03-02 01:38:32.669082 I | embed: ready to serve client requests
2019-03-02 01:38:32.670328 N | embed: serving insecure client requests on [::]:2379, this is strongly discouraged!
//}

次にetcdctlを用意します。etcdctlはetcdとやり取りするためのコマンドラインツールです。
以下のコマンドを実行すると、カレントディレクトリにetcdctlがコピーされます。

//cmd{
$ docker run --rm -u root:root \
  --entrypoint /usr/local/etcd/install-tools \
  --mount type=bind,src=$(pwd),target=/host \
  quay.io/cybozu/etcd:3.3
//}

//cmd{
$ ETCDCTL_API=3 ./etcdctl endpoint health
127.0.0.1:2379 is healthy: successfully committed proposal: took = 915.187µs
//}


//listnum[source-code][ソースコード]{
#@mapfile(../code/chapter1/client1/hello.go)
package main

import (
    "context"
    "fmt"
    "os"

    "github.com/coreos/etcd/clientv3"
)

func main() {
    fmt.Println("hello")
    cfg := clientv3.Config{
        Endpoints: []string{"http://localhost:2379"},
    }

    client, err := clientv3.New(cfg)
    if err != nil {
        fmt.Println(err)
        os.Exit(1)
    }

    key := "test"
    ctx := context.Background()
    resp, err := client.Get(ctx, key)
    if err != nil {
        fmt.Println(err)
        os.Exit(1)
    }
    if resp.Count == 0 {
        fmt.Printf("key not found: %s", key)
        os.Exit(1)
    }
    fmt.Println(resp.Kvs[0].Value)
}
#@end
//}
