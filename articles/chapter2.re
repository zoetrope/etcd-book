= etcdプログラミングの基本

== クライアントを用意する

//listnum[client][クライアントの作成]{
#@mapfile(../code/chapter2/client/client.go)
package main

import (
    "context"
    "fmt"
    "log"
    "time"

    "github.com/coreos/etcd/clientv3"
)

func main() {
    cfg := clientv3.Config{
        Endpoints:   []string{"http://localhost:2379"},
        DialTimeout: 3 * time.Second,
    }

    client, err := clientv3.New(cfg)
    if err != nil {
        log.Fatal(err)
    }
    defer client.Close()

    resp, err := client.Status(context.Background(), "localhost:2379")
    if err != nil {
        log.Fatal(err)
    }
    fmt.Printf("%#v\n", resp)
}
#@end
//}


== KV

//listnum[kv-write][データの書き込み]{
#@maprange(../code/chapter2/kv/kv.go,write)
    _, err = client.Put(context.TODO(), "/chapter2/kv", "value")
    if err != nil {
        log.Fatal(err)
    }
#@end
//}

//listnum[kv-read][データの読み込み]{
#@maprange(../code/chapter2/kv/kv.go,read)
    resp, err := client.Get(context.TODO(), "/chapter2/kv")
    if err != nil {
        log.Fatal(err)
    }
    if resp.Count == 0 {
        log.Fatal(err)
    }
    fmt.Println(string(resp.Kvs[0].Value))
#@end
//}

== Option

//listnum[opts-read][オプションの指定]{
#@maprange(../code/chapter2/opts/opts.go,read)
    client.Put(context.TODO(), "/chapter2/option/key3", "hoge")
    client.Put(context.TODO(), "/chapter2/option/key1", "foo")
    client.Put(context.TODO(), "/chapter2/option/key2", "bar")
    resp, err := client.Get(context.TODO(), "/chapter2/option/",
        clientv3.WithPrefix(),
        clientv3.WithSort(clientv3.SortByModRevision, clientv3.SortDescend),
        clientv3.WithKeysOnly(),
    )
    if err != nil {
        log.Fatal(err)
    }
    for _, kv := range resp.Kvs {
        fmt.Printf("%s: %s\n", kv.Key, kv.Value)
    }
#@end
//}

実行結果は次のようになります。

//terminal{
/chapter2/option/key2: 
/chapter2/option/key1: 
/chapter2/option/key3: 
//}

まず@<code>{WithPrefix}を指定すると、キーに指定したプレフィックス(ここでは/mykey/)から始まるキーをすべて取得しています。
次に@<code>{WithSort}を指定すると結果をソートすることができます。ここではModRevisionの降順、すなわち編集されたのが新しい順にキーを取得しています。
最後の@<code>{WithKeysOnly}を指定すると、キーのみを取得します。そのため結果に値が出力されていません。

この他にもキーの数だけを返す@<code>{WithCountOnly}や、見つかった最初のキーだけを返す@<code>{WithFirstKey}などたくさんのオプションがあります。

== Watch

== Lease

何に使えるか。
一時的にログを蓄えておきたい(Kubernetesのkube-apiserverの--event-ttlとか)
セッション情報とか

//listnum[lease][キーの有効期限を設定]{
#@maprange(../code/chapter2/lease/lease.go,lease)
    grantResp, err := client.Grant(context.TODO(), 5)
    if err != nil {
        log.Fatal(err)
    }
    _, err = client.Put(context.TODO(), "/chapter2/lease", "value", clientv3.WithLease(grantResp.ID))
    if err != nil {
        log.Fatal(err)
    }

    for {
        getResp, err := client.Get(context.TODO(), "/chapter2/lease")
        if err != nil {
            log.Fatal(err)
        }
        if getResp.Count == 0 {
            fmt.Println("'/chapter2/lease' disappeared")
            break
        }
        fmt.Printf("[%v] %s\n", time.Now().Format("15:04:05"), getResp.Kvs[0].Value)
        time.Sleep(1 * time.Second)
    }
#@end
//}

//terminal{
$ ./lease                   
[21:40:59] value
[21:41:00] value
[21:41:01] value
[21:41:02] value
[21:41:03] value
[21:41:04] value
'/chapter2/lease' disappeared
//}

//listnum[][リースの期限を延長する]{
    kaoResp, err = client.KeepAliveOnce(context.TODO(), grantResp.ID)
    if err != nil {
        log.Fatal(err)
    }
    fmt.Println(kao.TTL)
//}

//listnum[][リースの期限を延長し続ける]{
    _, err = client.KeepAlive(context.TODO(), grantResp.ID)
    if err != nil {
        log.Fatal(err)
    }
//}

//listnum[][リースを失効させる]{
    _, err = client.Revoke(context.TODO(), grantResp.ID)
    if err != nil {
        log.Fatal(err)
    }
//}

== Namespace
