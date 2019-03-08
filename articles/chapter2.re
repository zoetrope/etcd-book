= etcdプログラミング

== クライアントを用意する

//listnum[client][クライアントの作成]{
#@mapfile(../code/chapter2/client/client.go)
package main

import (
    "context"
    "fmt"
    "os"
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
        fmt.Println(err)
        os.Exit(1)
    }
    defer client.Close()

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
    _, err = client.Put(context.TODO(), "/chapter2/kv", "value")
    if err != nil {
        fmt.Println(err)
        os.Exit(1)
    }
#@end
//}

//listnum[kv-read][データの読み込み]{
#@maprange(../code/chapter2/kv/kv.go,read)
    resp, err := client.Get(context.TODO(), "/chapter2/kv")
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
        fmt.Println(err)
        os.Exit(1)
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

== Transaction


//listnum[tocttou][TOCTTOUの例]{
#@maprange(../code/chapter2/tocttou/tocttou.go,tocttou)
    client.Put(context.TODO(), "/chapter2/tocttou", "10")
    go func() {
        resp, _ := client.Get(context.TODO(), "/chapter2/tocttou")
        value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
        value += 5
        client.Put(context.TODO(), "/chapter2/tocttou", strconv.Itoa(value))
    }()
    go func() {
        resp, _ := client.Get(context.TODO(), "/chapter2/tocttou")
        value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
        value -= 3
        client.Put(context.TODO(), "/chapter2/tocttou", strconv.Itoa(value))
    }()
    time.Sleep(1 * time.Second)
    resp, _ := client.Get(context.TODO(), "/chapter2/tocttou")
    fmt.Println(string(resp.Kvs[0].Value))
#@end
//}

TOCTTOU(Time of check to time of use)

Transactionを利用したコードに書き換えてみましょう。

//listnum[txn][Transaction]{
#@maprange(../code/chapter2/transaction/transaction.go,txn)
    client.Put(context.TODO(), "/chapter2/txn", "10")
    go func() {
    RETRY:
        resp, _ := client.Get(context.TODO(), "/chapter2/txn")
        rev := resp.Kvs[0].ModRevision
        value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
        value += 5
        tresp, err := client.Txn(context.TODO()).
            If(clientv3.Compare(clientv3.ModRevision("/chapter2/txn"), "=", rev)).
            Then(clientv3.OpPut("/chapter2/txn", strconv.Itoa(value))).
            Commit()
        if err != nil {
            return
        }
        if !tresp.Succeeded {
            goto RETRY
        }
    }()
    go func() {
    RETRY:
        resp, _ := client.Get(context.TODO(), "/chapter2/txn")
        rev := resp.Kvs[0].ModRevision
        value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
        value -= 3
        tresp, err := client.Txn(context.TODO()).
            If(clientv3.Compare(clientv3.ModRevision("/chapter2/txn"), "=", rev)).
            Then(clientv3.OpPut("/chapter2/txn", strconv.Itoa(value))).
            Commit()
        if err != nil {
            return
        }
        if !tresp.Succeeded {
            goto RETRY
        }
    }()
    time.Sleep(1 * time.Second)
    resp, _ := client.Get(context.TODO(), "/chapter2/txn")
    fmt.Println(string(resp.Kvs[0].Value))
#@end
//}

Mutex.IsOwner

== Lease

== Watch

== Concurrency

MVCC
