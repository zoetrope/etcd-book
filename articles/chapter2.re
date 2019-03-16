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

    resp, err := client.MemberList(context.TODO())
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

//footnote[package][etcd v3.3までは@<code>{github.com/coreos/etcd/clientv3}ですが、etcd v3.4から@<code>{github.com/etcd-io/etcd/clientv3}に変わるので注意してください。]

次に@<code>{clientv3.Config}型の変数を用意しています。
これはetcdに接続するときに利用する設定で、接続先のアドレスや接続時のタイムアウト時間などを指定できます。

この設定を利用して、@<code>{clientv3.New()}でクライアントを作成します。
このとき、etcdとの接続に失敗するとエラーが返ってきます。

最後に作成したクライアントを利用して、@<code>{MemberList()}

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

== context

etcdのクライアントが提供している多くのメソッドは、第一引数に@<code>{context.Context}を受けるようになっています。

@<code>{context.Context}は、ブロッキング処理や時間のかかる処理など、
タイムアウトさせたり、キャンセルさせるために利用される機構です@<fn>{context}。

//footnote[context][context.Contextはキャンセルの伝搬だけでなく値の共有にも利用できますが、ここでは説明は割愛します。]

etcdクライアントは、etcdサーバーとの通信をおこなうため、
リクエストを投げてからどれくらいの時間で返ってくるかはわかりません。その間の処理はブロックされることになります。

それではcontextを利用して処理をキャンセルする例をみてみましょう。

//listnum[context][処理のキャンセル]{
#@maprange(../code/chapter2/timeout/timeout.go,timeout)
    ctx, cancel := context.WithTimeout(context.Background(), 1*time.Second)
    defer cancel()
    resp, err := client.Get(ctx, "/chapter2/timeout")
    if err != nil {
        log.Fatal(err)
    }
    fmt.Println(resp)
#@end
//}

この例では1秒でタイムアウトするcontextを作り、@<code>{client.Get()}の第一引数に渡しています。
これにより、@<code>{client.Get()}に時間がかかっていた場合、1秒経過すると途中で処理を終了します。
処理が途中で終了した場合、@<code>{client.Get()}はエラーを返します。

このプログラムを実行してみましょう。
通常はGetの処理に1秒もかからないため、問題なく値が取得できていると思います。

それではネットワーク遅延を擬似的に発生させてみましょう。
詳しくは@<chap>{chapter6}で説明しますが、以下のようにtcコマンドを利用することで、etcdコンテナとの通信に3秒の遅延が発生します。

//terminal{
$ VETH=$(sudo ./chapter6/veth.sh etcd)
$ sudo tc qdisc add dev $VETH root netem delay 3s
//}

もう一度プログラムを実行してみましょう。
少し待つと"context deadline exceedef"というメッセージが表示されてプログラムが終了するはずです。

//terminal{
$ VETH=$(sudo ./chapter6/veth.sh etcd)
$ sudo tc qdisc del dev $VETH root
//}




本書では@<code>{context.TODO()}を利用していますが、実際にコードを書くときには適切なコンテキストを利用してください@<fn>{todo}。

//footnote[todo][TODOという文字列が残っているとエディタや静的解析ツールが注意してくれるので、それに気づくことができるでしょう。]

== Watch

//listnum[watch][変更の監視]{
#@maprange(../code/chapter2/watch/watch.go,watch)
    ch := client.Watch(context.TODO(), "/chapter2/watch/", clientv3.WithPrefix())
    for resp := range ch {
        if resp.Err() != nil {
            log.Fatal(resp.Err())
        }
        for _, ev := range resp.Events {
            switch ev.Type {
            case clientv3.EventTypePut:
                switch {
                case ev.IsCreate():
                    fmt.Printf("CREATE %q : %q\n", ev.Kv.Key, ev.Kv.Value)
                case ev.IsModify():
                    fmt.Printf("MODIFY %q : %q\n", ev.Kv.Key, ev.Kv.Value)
                }
            case clientv3.EventTypeDelete:
                fmt.Printf("DELETE %q : %q\n", ev.Kv.Key, ev.Kv.Value)
            }
        }
    }
#@end
//}

Errを返すケース
* クライアントをCloseするなど監視を終了した場合
* Compactionされた場合

WithPrefix
WithRev
WithPrevKV
WithProgressNotify
WithCreatedNotify
WithFilterPut
WithFilterDelete

まずGetで値を取ってきて処理。その後にwatchを開始。
GetとWatchの間にもし値が変更されていたら、値を取りこぼすことになってしまいます。
データを取りこぼさないようにWithRevを利用する。
最後に読み取ったrevの値をファイルなどに書き出しておいてもよいでしょう。

revを指定してWatchを開始しした場合、その値はすでにコンパクションされている可能性もあります。

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

リースの期間を延長し続けるなんて、リースの意味がなくなってしまうのではと思うかもしれません。
しかし、KeepAliveを呼び出したプログラムが終了すると、リースの期限は延長されなくなり、いずれそのキーは消えてしまいます。
この機能を利用することでプログラムの生存をチェックすることが可能になります。
これをうまく利用したのがリーダー選出の機能です。後ほど紹介します。

//listnum[][リースの期限を延長し続ける]{
    _, err = client.KeepAlive(context.TODO(), grantResp.ID)
    if err != nil {
        log.Fatal(err)
    }
//}

指定した期限までまだ時間がある場合でも、そのキーを失効させたい場合があります。

//listnum[][リースを失効させる]{
    _, err = client.Revoke(context.TODO(), grantResp.ID)
    if err != nil {
        log.Fatal(err)
    }
//}

== Namespace
