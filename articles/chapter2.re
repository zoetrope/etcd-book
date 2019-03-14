= etcdプログラミングの基本

== クライアントを用意する

それではさっそくGo言語によるetcdプログラミングをはじめましょう。

Go言語の開発環境はセットアップ済みですか？
もしセットアップが完了していないのであれば、@<href>{https://golang.org/doc/install}を開き、
利用しているOS向けのバイナリをダウンロードしてきてください。

Go言語のバージョンを確認しておきましょうか。
本書ではGo 1.12を利用します。
本書のサンプルコードは古いGoでも動くとは思いますが、最新版を利用することをおすすめします。

//terminal{
$ go version
go version go1.12 linux/amd64
//}

それではetcdにアクセスするGo言語のプログラムを書いてみましょう。
以下のようなファイルを作成し、client.goという名前で保存してください。

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

    resp, err := client.MemberList(context.Background())
    if err != nil {
        log.Fatal(err)
    }
    for _, m := range resp.Members {
        fmt.Printf("%s\n", m.String())
    }
}
#@end
//}

次にこのコードをコンパイルします。何もエラーメッセージが表示されなければ成功です。

//terminal{
$ go build client.go
//}

etcdは起動したままになっていますか？ 
もし起動していないようなら@<chap>{chapter1}に戻って起動してきてください。

コンパイルしたプログラムを実行してみましょう。
以下のようなメッセージが表示されれば成功です。
このプログラムでは、etcdクラスタを構成するメンバーの情報を表示しています。

//terminal{
$ ./client
ID:10276657743932975437 name:"etcd-1" peerURLs:"http://localhost:2380" clientURLs:"http://0.0.0.0:2379"
//}

もしetcdが起動していなかったり、接続するアドレスが間違っていたりすると、以下のようなエラーが発生します。
etcdが起動しているかどうか、プログラムが間違っていないかどうかを確認してください。

//terminal{
$ ./client
2019/03/14 20:34:02 dial tcp 127.0.0.1:2379: connect: connection refused
//}

ではソースコードの解説をしていきます。

まず@<code>{github.com/coreos/etcd/clientv3}をインポートしています@<fn>{package}。これは

次に@<code>{clientv3.Config}型の変数を用意しています。
これはetcdに接続するときに利用する設定で、接続先のアドレスや接続時のタイムアウト時間などを指定できます。



//footnote[package][etcd v3.3までは@<code>{github.com/coreos/etcd/clientv3}ですが、etcd v3.4から@<code>{github.com/etcd-io/etcd/clientv3}に変わるので注意してください。]

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
    _, err = client.Put(context.TODO(), "/chapter2/lease", "value",
        clientv3.WithLease(grantResp.ID))
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
        fmt.Printf("[%v] %s\n",
            time.Now().Format("15:04:05"), getResp.Kvs[0].Value)
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
