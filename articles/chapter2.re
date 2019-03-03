= etcdプログラミング

== クライアントを用意する

//listnum[client][クライアントの作成]{
#@mapfile(../code/chapter2/client/client.go)
package main

import (
    "context"
    "fmt"
    "os"

    "github.com/coreos/etcd/clientv3"
)

func main() {
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
        fmt.Printf("key not found: %s\n", key)
        os.Exit(1)
    }
    fmt.Println(resp.Kvs[0].Value)
}
#@end
//}


== KV

//listnum[kv-write][データの書き込み]{
#@mapfile(../code/chapter2/kv/write.go)
package main

import (
    "context"

    "github.com/coreos/etcd/clientv3"
)

func write(client *clientv3.Client) error {
    key := "test"
    ctx := context.Background()
    _, err := client.Put(ctx, key, "value")
    if err != nil {
        return err
    }
    return nil
}
#@end
//}

//listnum[kv-read][データの読み込み]{
#@mapfile(../code/chapter2/kv/read.go)
package main

import (
    "context"
    "fmt"

    "github.com/coreos/etcd/clientv3"
)

func read(client *clientv3.Client) error {
    key := "test"
    ctx := context.Background()
    resp, err := client.Get(ctx, key)
    if err != nil {
        return err
    }
    if resp.Count == 0 {
        return err
    }
    fmt.Println(resp.Kvs[0].Value)
    return nil
}
#@end
//}

* Version
* Revision
* ModRevision
* CreateRevision

== Transaction

== Lease

== Watch

== Concurrency
