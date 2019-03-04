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

    resp, err := client.Status(context.Background(), "localhost:2379")
    if err != nil {
        fmt.Println(err)
        os.Exit(1)
    }
    fmt.Printf("%#v\n", resp)
}
#@end
//}


== KV

//listnum[kv-write][データの書き込み]{
#@maprange(../code/chapter2/kv/kv.go,write)
    _, err = client.Put(context.Background(), "/mykey", "value")
    if err != nil {
        fmt.Println(err)
        os.Exit(1)
    }
#@end
//}

//listnum[kv-read][データの読み込み]{
#@maprange(../code/chapter2/kv/kv.go,read)
    resp, err := client.Get(context.Background(), "/mykey")
    if err != nil {
        fmt.Println(err)
        os.Exit(1)
    }
    if resp.Count == 0 {
        fmt.Println("not found")
        os.Exit(1)
    }
    fmt.Println(string(resp.Kvs[0].Value))
#@end
//}


== Transaction

TOCTTOU(Time of check to time of use)

Mutex.IsOwner

== Lease

== Watch

== Concurrency
