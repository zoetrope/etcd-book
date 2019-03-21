= etcdプログラミングの基本

本章以降ではetcdのクライアントライブラリを利用して、Go言語でetcdを操作するプログラムの書き方を紹介していきます。
ここではまず、etcdに接続するクライアントをの作り方と、それを利用したキー・バリューの読み書きの方法、
さらにキー・バリューの変更を監視する方法や、キーに有効期限を指定する方法を紹介します。

== クライアントを用意する

それではさっそくGo言語によるetcdプログラミングをはじめましょう。

Go言語の開発環境はセットアップ済みですか？
もしセットアップが完了していないのであれば、@<href>{https://golang.org/doc/install}を開き、利用しているOS向けのバイナリをダウンロードしてきてください。

Goのバージョンを確認しておきましょうか。

//terminal{
$ go version
go version go1.12.1 linux/amd64
//}

本書ではGo 1.12.1を利用します。
本書のコードは古いGoでも動くとは思いますが、最新版を利用することをおすすめします。

ではetcdにアクセスするプログラムを書いてみましょう。
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

次にこのコードをコンパイルします。何もエラーメッセージが表示されず、実行ファイルが生成されていれば成功です。

//terminal{
$ go build client.go
//}

etcdは起動したままになっていますか？ 
もし起動していないようなら@<chap>{chapter1}に戻って起動してきてください。

etcdが起動しているコンピュータ上で、コンパイルしたプログラムを実行してみましょう。
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

まず@<code>{github.com/coreos/etcd/clientv3}をインポートしています。
これによりetcdのクライアントライブラリが利用できるようになります。

//list[import][]{
import (
    "github.com/coreos/etcd/clientv3"
)
//}

//note[パッケージ名が変わります]{
etcd v3.3までのパッケージ名は@<code>{github.com/coreos/etcd/clientv3}ですが、etcd v3.4から@<code>{github.com/etcd-io/etcd/clientv3}に変わるので注意してください。
//}

次に@<code>{main}関数の中を見ていきましょう。
@<code>{clientv3.Config}型の変数を用意しています。
これはetcdに接続するときに利用する設定で、接続先のアドレスや接続時のタイムアウト時間などを指定できます。
ここではlocalhost上の2379番ポートに接続し、接続タイムアウト時間が3秒になるように設定しています。

//list[config][]{
cfg := clientv3.Config{
    Endpoints:   []string{"http://localhost:2379"},
    DialTimeout: 3 * time.Second,
}
//}

この設定を利用して、@<code>{clientv3.New()}でクライアントを作成します。
このとき、etcdとの接続に失敗するとエラーが返ってくるのでエラー処理をしましょう。
また、etcdとのやり取りが完了したときに@<code>{client.Close()}を呼び出すようにしておきます。

//list[client.new][]{
client, err := clientv3.New(cfg)
if err != nil {
    log.Fatal(err)
}
defer client.Close()
//}

最後に作成したクライアントを利用して、@<code>{MemberList()}を呼び出し、その結果を画面に表示しています。
このとき引数に@<code>{context.TODO()}を渡していますが、@<code>{context}については後ほど詳しく解説します。

//list[memberlist][]{
resp, err := client.MemberList(context.TODO())
if err != nil {
    log.Fatal(err)
}
for _, m := range resp.Members {
    fmt.Printf("%s\n", m.String())
}
//}

このように、クライアントを作成しそれを利用して必要なAPIを呼び出すのが、etcdプログラミングの基本的な流れになります。

== Key Value
では、先ほど作成したクライアントを利用して、etcdにデータを書き込んでみましょう。

@<list>{client}のファイルをコピーし、@<code>{MemberList()}を呼び出す代わりに以下のようなコードを書いてみます。

//listnum[kv-write][データの書き込み]{
#@maprange(../code/chapter2/kv/kv.go,write)
    _, err = client.Put(context.TODO(), "/chapter2/kv", "my-value")
    if err != nil {
        log.Fatal(err)
    }
#@end
//}

これを実行すると@<code>{/chapter2/kv}というキーに@<code>{my-value}という値が書き込まれます。
なお、@<code>{Put}を利用すると、指定したキーがなければ新しくデータが作成され、キーが存在していればその値が更新されることになります。

では、書き込んだ値が正しく読み込めるかどうか試してみましょう。
先程のコードの続きに以下の処理を追加します。

//listnum[kv-read][データの読み込み]{
#@maprange(../code/chapter2/kv/kv.go,read)
    resp, err := client.Get(context.TODO(), "/chapter2/kv")
    if err != nil {
        log.Fatal(err)
    }
    if resp.Count == 0 {
        log.Fatal("/chapter2/kv not found")
    }
    fmt.Println(string(resp.Kvs[0].Value))
#@end
//}

このコードを実行すると、書き込んだ値が読み込まれて@<code>{my-value}が表示されることでしょう。
もし書き込みや読み込みが失敗した場合は何らかのエラーメッセージが表示されます。

次に書き込んだ値を削除してみましょう。

//listnum[kv-delete][データの削除]{
#@maprange(../code/chapter2/kv/kv.go,delete)
    _, err = client.Delete(context.TODO(), "/chapter2/kv")
    if err != nil {
        log.Fatal(err)
    }
#@end
//}

値を削除した後に読み込もうとしても、キーが見つからないという結果になるでしょう。

== Option

@<code>{client.Put()}, @<code>{client.Get()}, @<code>{client.Delete()}にはいろいろなオプションを指定することができます。
ここでは@<code>{client.Get()}を例に上げていくつかのオプションをみてみましょう。

//listnum[opts-read][オプションの指定]{
#@maprange(../code/chapter2/opts/opts.go,read)
    client.Put(context.TODO(), "/chapter2/option/key3", "val2")
    client.Put(context.TODO(), "/chapter2/option/key1", "val3")
    client.Put(context.TODO(), "/chapter2/option/key2", "val1")
    resp, err := client.Get(context.TODO(), "/chapter2/option/",
        clientv3.WithPrefix(),
        clientv3.WithSort(clientv3.SortByValue, clientv3.SortAscend),
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

このコードの実行結果は次のようになります。

//terminal{
/chapter2/option/key2: 
/chapter2/option/key3: 
/chapter2/option/key1: 
//}

まず@<code>{WithPrefix}を指定すると、キーに指定したプレフィックス(ここでは@<code>{/chapter2/option})から始まるキーをすべて取得します。
次に@<code>{WithSort}を指定すると結果をソートすることができます。ここではValueの昇順で並び替えをおこないます。
最後の@<code>{WithKeysOnly}を指定すると、キーのみを取得します。そのため結果に値が出力されていません。

この他にもキーの数だけを返す@<code>{WithCountOnly}や、見つかった最初のキーだけを返す@<code>{WithFirstKey}などたくさんのオプションがあります。
ぜひ@<href>{https://godoc.org/github.com/coreos/etcd/clientv3#OpOption}を一度みてください。

== MVCC (MultiVersion Concurrency Control)

etcdはMVCC (MultiVersion Concurrency Control)モデルを採用したキーバリューストアです。
すなわち、etcdに対して実施した変更はすべて履歴が保存されており、それぞれにリビジョン番号がつけられています。
リビジョン番号を指定して値を読み出せば、過去の時点での値も読み出すことができます。

ただし、すべての履歴を保存しているとディスク容量が逼迫してしまうため、適当なタイミングでコンパクションして古い履歴を削除するのが一般的です。

では、具体的にリビジョン番号が更新されていく様子を見てみましょう。

その前に@<code>{client.Get()}の結果を詳しく表示するヘルパー関数を用意しておきます。

//listnum[mvcc-print][]{
#@maprange(../code/chapter2/revision/revision.go,print)
func printResponse(resp *clientv3.GetResponse) {
    fmt.Println("header: " + resp.Header.String())
    for i, kv := range resp.Kvs {
        fmt.Printf("kv[%d]: %s\n", i, kv.String())
    }
    fmt.Println()
}

#@end
//}

では書き込んだ値を取得して、その結果を表示してみましょう。

//list[mvcc-rev1][]{
#@maprange(../code/chapter2/revision/revision.go,rev1)
    client.Put(context.TODO(), "/chapter2/rev/1", "123")
    resp, _ := client.Get(context.TODO(), "/chapter2/rev", clientv3.WithPrefix())
    printResponse(resp)
#@end
//}

実行すると以下のように表示されます。

//terminal{
header: cluster_id:14841639068965178418 member_id:10276657743932975437 revision:41 raft_term:2 
kv[0]: key:"/chapter2/rev/1" create_revision:41 mod_revision:41 version:1 value:"123" 
//}

@<code>{header}の中に@<code>{revision}というフィールドが見つかります。

: revision
    etcdのリビジョン番号。クラスタ全体で一つの値が利用される。etcdに何らかの変更(キーの追加、変更、削除)が加えられると値が1増える。

キーバリューの中には以下のフィールドが見つかります。

: create_revision
    このキーが作成されたときのリビジョン番号。
: mod_revision
    このキーの内容が最後に変更されたときのリビジョン番号。
: version
    このキーのバージョン。このキーに変更が加えられると値が1増える。

次に@<code>{/chapter2/rev/1}の値を更新してみましょう。

//list[mvcc-rev2][]{
#@maprange(../code/chapter2/revision/revision.go,rev2)
    client.Put(context.TODO(), "/chapter2/rev/1", "456")
    resp, _ = client.Get(context.TODO(), "/chapter2/rev", clientv3.WithPrefix())
    printResponse(resp)
#@end
//}

@<code>{revision}, @<code>{mod_revision}, @<code>{version}の値が1つ増え、
@<code>{create_revision}はそのままの値になっています。

//terminal{
header: cluster_id:14841639068965178418 member_id:10276657743932975437 revision:42 raft_term:2 
kv[0]: key:"/chapter2/rev/1" create_revision:41 mod_revision:42 version:2 value:"456" 
//}

次に別のキー@<code>{/chapter2/rev/2}に値を書き込んでみましょう。

//list[mvcc-rev3][]{
#@maprange(../code/chapter2/revision/revision.go,rev3)
    client.Put(context.TODO(), "/chapter2/rev/2", "999")
    resp, _ = client.Get(context.TODO(), "/chapter2/rev", clientv3.WithPrefix())
    printResponse(resp)
#@end
//}

このときの情報を見ると@<code>{header}の@<code>{revision}は増加し、
@<code>{/chapter2/rev/1}の@<code>{mod_revision}, @<code>{version}, @<code>{create_revision}は値が変化していません。
一方、@<code>{/chapter2/rev/2}の@<code>{mod_revision}, @<code>{create_revision}は最新の@<code>{revision}になり、
@<code>{version}は1になっていることがわかります。

//terminal{
header: cluster_id:14841639068965178418 member_id:10276657743932975437 revision:43 raft_term:2 
kv[0]: key:"/chapter2/rev/1" create_revision:41 mod_revision:42 version:2 value:"456" 
kv[1]: key:"/chapter2/rev/2" create_revision:43 mod_revision:43 version:1 value:"999" 
//}

続いて@<code>{/chapter2/rev/1}が作成された時点でのリビジョン番号(41)を指定して値を読み出してみましょう。
リビジョンを指定して値を読み出すためには、@<code>{clientv3.WithRev()}を利用します。

//list[mvcc-withrev][]{
#@maprange(../code/chapter2/revision/revision.go,withrev)
    resp, _ = client.Get(context.TODO(), "/chapter2/rev",
        clientv3.WithPrefix(), clientv3.WithRev(resp.Kvs[0].CreateRevision))
    printResponse(resp)
#@end
//}

実行すると@<code>{/chapter2/rev/1}が作成されたときの値@<code>{123}が取得できています。
このリビジョンの時点では@<code>{/chapter2/rev/2}は存在しなかったため、その値は取得できませんでした。
またこのとき@<code>{header}の@<code>{revision}は、指定したリビジョンではなく最新のリビジョン番号が入っていることに注意しましょう。

//terminal{
header: cluster_id:14841639068965178418 member_id:10276657743932975437 revision:43 raft_term:2 
kv[0]: key:"/chapter2/rev/1" create_revision:41 mod_revision:41 version:1 value:"123" 
//}

ここで説明したリビジョンは、後ほど解説するWatchやTransactionでも利用することになります。

== context

etcdのクライアントが提供している多くのメソッドは、第一引数に@<code>{context.Context}を受けるようになっています。

@<code>{context.Context}は、Go言語においてブロッキング処理などの時間のかかる処理を、タイムアウトさせたりキャンセルさせるために利用される機構です。

etcdクライアントは、etcdサーバーとの通信をおこなうため、
リクエストを投げてからどれくらいの時間で返ってくるかはわかりません。その間の処理はブロックされることになります。
そのため、処理を中断したいようなケースでは@<code>{context}を利用することになります。

それでは@<code>{context}を利用して処理をキャンセルする例をみてみましょう。

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

この例では1秒でタイムアウトする@<code>{context}を作り、@<code>{client.Get()}の第一引数に渡しています。
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
少し待つと"context deadline exceeded"というメッセージが表示されてプログラムが終了するはずです。
このようなタイムアウト処理を利用すると、例えばネットワークの調子が悪いときに、処理を中断して管理者に通知を飛ばすような仕組みを構築することも可能になります。

遅延させたままでは困るので、もとに戻しておきましょう。

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


//listnum[watch_rev][変更の監視]{
#@maprange(../code/chapter2/watch_rev/watch_rev.go,watch_rev)
    resp, err := client.Get(context.TODO(), "/chapter2/watch_rev")
    if err != nil || resp.Count == 0 {
        log.Fatal(err)
    }
    fmt.Printf("processed: %s\n", resp.Kvs[0].Value)
    rev := resp.Kvs[0].ModRevision
    time.Sleep(300 * time.Millisecond)
    ch := client.Watch(context.TODO(), "/chapter2/watch_rev", clientv3.WithRev(rev+1))
    for resp := range ch {
        if resp.Err() != nil {
            log.Fatal(resp.Err())
        }
        for _, ev := range resp.Events {
            fmt.Printf("%s %q : %q\n", ev.Type, ev.Kv.Key, ev.Kv.Value)
        }
    }
#@end
//}
まずGetで値を取ってきて処理。その後にwatchを開始。
Watchを呼び出すと、呼び出した時点からの変更が通知されることになります。
Getで処理をしてからWatchを呼び出すまでの間にもし値が変更されていたら、その変更を取りこぼすことになってしまいます。
データを取りこぼさないようにWithRevを利用する。
最後に読み取ったrevの値をファイルなどに書き出しておいてもよいでしょう。

プログラムを一旦停止して、前回の続きから処理を再開したいケースもある

revを指定してWatchを開始しした場合、その値はすでにコンパクションされている可能性もあります。

== Lease

何に使えるか。
一時的にログを蓄えておきたい(Kubernetesのkube-apiserverの--event-ttlとか)
セッション情報とか
後述するMutexやLeader Electionでも使ってる。

//listnum[lease][キーの有効期限を設定]{
#@maprange(../code/chapter2/lease/lease.go,lease)
    lease, err := client.Grant(context.TODO(), 5)
    if err != nil {
        log.Fatal(err)
    }
    _, err = client.Put(context.TODO(), "/chapter2/lease", "value",
        clientv3.WithLease(lease.ID))
    if err != nil {
        log.Fatal(err)
    }

    for {
        resp, err := client.Get(context.TODO(), "/chapter2/lease")
        if err != nil {
            log.Fatal(err)
        }
        if resp.Count == 0 {
            fmt.Println("'/chapter2/lease' disappeared")
            break
        }
        fmt.Printf("[%v] %s\n",
            time.Now().Format("15:04:05"), resp.Kvs[0].Value)
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

