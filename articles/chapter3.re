= etcdによる分散処理プログラミング

前章ではGo言語を利用してetcdの基本的なデータの読み書きなどの操作をおこないました。
本章ではトランザクション処理や、分散システムを開発する上で必要となる分散ロックやリーダー選出のプログラミング方法を紹介していきます。

== Transaction

etcdにアクセスするクライアントが常に1つしか存在しないのであれば何も問題はありません。
しかし、現実には複数のクライアントが同時にetcdにデータを書き込んだり読み込んだりします。
このようなとき、正しくデータの読み書きをおこなわないと、データの不整合が発生する可能性があります。
例えば、以下のような例をみてみましょう。

etcdから現在の値を読み取り、そこに引数で指定した値を追加して保存するという単純な@<code>{addValue}という関数を用意します。

//list[?][]{
#@maprange(../code/chapter3/conflict/conflict.go,add)
func addValue(client *clientv3.Client, key string, d int) {
    resp, _ := client.Get(context.TODO(), key)
    value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
    value += d
    client.Put(context.TODO(), key, strconv.Itoa(value))
}

#@end
//}

@<code>{/chapter/conflict}というキーに10をセットします。
その後に、先程の@<code>{addValue()}関数に5と-3を渡してgoroutineとして並列に実行します。

//list[?][]{
#@maprange(../code/chapter3/conflict/conflict.go,conflict)
    key := "/chapter3/conflict"
    client.Put(context.TODO(), key, "10")
    go addValue(client, key, 5)
    go addValue(client, key, -3)
    time.Sleep(1 * time.Second)
    resp, _ := client.Get(context.TODO(), key)
    fmt.Println(string(resp.Kvs[0].Value))
#@end
//}

このコードの実行結果は12になってほしいところです。
しかし実際に実行してみると、結果は15になったり7になったりばらつきます。
片方のgoroutineで値を読み込んでから書き込むまでの間に、もう一方のgoroutineが値を書き換えてしまっているためにこのような問題が発生します。

ではこの関数をトランザクションを利用して、問題が起きないように書き換えてみましょう。

//list[txn][Transaction]{
#@maprange(../code/chapter3/transaction/transaction.go,txn)
func addValue(client *clientv3.Client, key string, d int) {
RETRY:
    resp, _ := client.Get(context.TODO(), key)
    rev := resp.Kvs[0].ModRevision
    value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
    value += d
    tresp, err := client.Txn(context.TODO()).
        If(clientv3.Compare(clientv3.ModRevision(key), "=", rev)).
        Then(clientv3.OpPut(key, strconv.Itoa(value))).
        Commit()
    if err != nil {
        log.Fatal(err)
    }
    if !tresp.Succeeded {
        goto RETRY
    }
}

#@end
//}

このコードを実行すると結果は必ず12になり、期待する結果が得られます。
最初のコードと比べると複雑になっていますが、順に解説していきます。

まず、@<code>{rev := resp.Kvs[0].ModRevision}で、指定した値が最後に変更されたときのリビジョンを取得します。

つぎに@<code>{client.Txn()}メソッドを利用して
@<code>{If(clientv3.Compare(clientv3.ModRevision("/chapter3/txn"), "=", rev))}では、
現在の/chapter3/txnのModRevisionと、最初に値を取得したときのModRevisionを比較しています。
すなわち、値を取得したときと現在で/chapter3/txnの値が書き換えられていないかどうかをチェックしています。

このifの条件が成立した場合にだけThenで指定した処理が実行されます。
ここでは

そして最後にtresp.Succeededをチェックしています。
この値はIfの条件が成立した場合にtrueになります。

=== いろいろなif条件

@<code>{If(clientv3.Compare(clientv3.ModRevision("/chapter3/txn"), "=", rev))}

 ** ターゲット
 *** Value
 *** Version
 *** CreateRevision
 *** ModRevision
 *** LeaseValue
 ** 演算子
 *** "="
 *** "!="
 *** "<"
 *** ">"
 ** KeyMissing
 ** KeyExists
=== Else
=== 複数の処理を同時実行するだけ
 ** OpGet
 ** OpPut
 ** OpDelete
 ** OpTxn
 ** OpCompact
=== ネストしたトランザクション

== Concurrency

利用するためには@<code>{"github.com/coreos/etcd/clientv3/concurrency"}パッケージをインポートする必要があります。

Sessionは、Lease機能をいいかんじに使えるようにしたClientのラッパーみたいなもの？
MutexやLeaderElectionで利用する。

=== Mutex

//list[mutex][ロック]{
#@maprange(../code/chapter3/mutex/mutex.go,lock)
    s, err := concurrency.NewSession(client)
    if err != nil {
        log.Fatal(err)
    }
    m := concurrency.NewMutex(s, "/chapter3/mutex")
    err = m.Lock(context.TODO())
    if err != nil {
        log.Fatal(err)
    }
    fmt.Println("acquired lock")
    time.Sleep(5 * time.Second)
    err = m.Unlock(context.TODO())
    if err != nil {
        log.Fatal(err)
    }
    fmt.Println("released lock")
#@end
//}

ロックを取ったままプログラムが終了してしまったら？
リースが設定してあるので大丈夫。

さて、ロックが取れたからといって安心してはいけません。
OSが提供するMutexとは異なり
ロックした後に一度ネットワーク接続が切れて、ロックのキーのリース期間が終了しているかもしれません。
そうなってしまうと、プログラムは自分がロックしたつもりで動作しているのに、実際にはロックされていないという状況に陥ってしまいます。

そこで、ロックを取ったあとにetcdのキーバリューの操作をおこなう際には、トランザクションを利用してIf条件に@<code>{Mutex.IsOwner()}を指定しましょう。

//list[owner][IsOwner]{
#@maprange(../code/chapter3/mutex/mutex.go,owner)
RETRY:
    select {
    case <-s.Done():
        log.Fatal("session has been orphaned")
    default:
    }
    err = m.Lock(context.TODO())
    if err != nil {
        log.Fatal(err)
    }
    resp, err := client.Txn(context.TODO()).
        If(m.IsOwner()).
        Then(clientv3.OpPut("/chapter3/mutex/owner", "test")).
        Commit()
    if err != nil {
        log.Fatal(err)
    }
    if resp.Succeeded {
        fmt.Println("the lock was not acquired")
        m.Unlock(context.TODO())
        goto RETRY
    }
    // do something
    err = m.Unlock(context.TODO())
    if err != nil {
        log.Fatal(err)
    }
#@end
//}


=== STM(Software Transactional Memory)

先程のトランザクション処理をSTMを使って書き換えてみましょう。

//list[stm][STM]{
#@maprange(../code/chapter3/stm/stm.go,stm)
    addValue := func(d int) {
        _, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
            v := stm.Get("/chapter3/stm")
            value, err := strconv.Atoi(v)
            if err != nil {
                return err
            }
            value += d
            stm.Put("/chapter3/stm", strconv.Itoa(value))
            return nil
        })
        if err != nil {
            log.Fatal(err)
        }
    }

    client.Put(context.TODO(), "/chapter3/stm", "10")
    go addValue(5)
    go addValue(-3)

    time.Sleep(1 * time.Second)
    resp, _ := client.Get(context.TODO(), "/chapter3/stm")
    fmt.Println(string(resp.Kvs[0].Value))
#@end
//}

WithIsolation
WithPrefetch
WithAbortContext

副作用禁止

 * SerializableSnapshot
 ** 分離レベルをSerializable
 ** さらに、最初に値を読み込んだとき
 * Serializable
 ** 初回のGet時のrevを覚えておく。2回目以降のGetは他のキーの場合でも同じrevを使って読み込む。
 * RepeatableReads
 * ReadCommitted
 ** 一般的なトランザクション分離レベルのRead Committedにはなっていない。
 ** 一切トランザクションになっていないので使うべきではない。
 ** ファジーリードも発生しない。一度readした値はキャッシュしているので必ず同じ値を返す。

//list[phantom][ファントムリード]{
#@maprange(../code/chapter3/isolation/isolation.go,phantom)
    addValue := func(d int) {
        _, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
            v1 := stm.Get("/chapter3/iso/phantom/key1")
            value := 0
            if len(v1) != 0 {
                value, _ = strconv.Atoi(v1)
            }
            value += d
            time.Sleep(time.Duration(rand.Intn(3)) * time.Second)
            v2 := stm.Get("/chapter3/iso/phantom/key2")
            if v1 != v2 {
                fmt.Printf("phantom:%d, %s, %s\n", d, v1, v2)
            }
            stm.Put("/chapter3/iso/phantom/key1", strconv.Itoa(value))
            stm.Put("/chapter3/iso/phantom/key2", strconv.Itoa(value))
            return nil
        }, concurrency.WithIsolation(concurrency.RepeatableReads))
        if err != nil {
            log.Fatal(err)
        }
    }

    client.Delete(context.TODO(), "/chapter3/iso/phantom/", clientv3.WithPrefix())
    go addValue(5)
    go addValue(-3)

    time.Sleep(5 * time.Second)
    resp, _ := client.Get(context.TODO(), "/chapter3/iso/phantom/key1")
    fmt.Println(string(resp.Kvs[0].Value))
#@end
//}

=== Leader Election

//list[leader][リーダー選出]{
#@maprange(../code/chapter3/leader/leader.go,leader)
    flag.Parse()
    if flag.NArg() != 1 {
        log.Fatal("usage: ./leader NAME")
    }
    name := flag.Arg(0)
    s, err := concurrency.NewSession(client)
    if err != nil {
        log.Fatal(err)
    }
    defer s.Close()
    e := concurrency.NewElection(s, "/chapter3/leader")

    err = e.Campaign(context.TODO(), name)
    if err != nil {
        log.Fatal(err)
    }
    for i := 0; i < 5; i++ {
        fmt.Println(name + " is a leader.")
        time.Sleep(1 * time.Second)
    }
    err = e.Resign(context.TODO())
    if err != nil {
        log.Fatal(err)
    }
#@end
//}

ではここで、リーダーが明示的にResignせずに終了してしまったらどうなるでしょうか？
Ctrl+Cを押してリーダーのプロセスを終了させてみてください。
他のプロセスがリーダーになることはなく、
その状態で60秒ほど待つとようやく他のプロセスがリーダーになります。

これはLease機能を利用して、リーダーキーのリース期間が延長されない場合は自動的にキーが削除されるようになっています。

リーダーになったからといって安心してはいけません。
ネットワーク障害が発生したりして、リーダーキーのリース期間が終了しているかもしれません。
自分がリーダーだと思って行動していたのに実はリーダーではなかったという状況に陥ります。
そこで、トランザクションのIf条件にリーダーキーが消えていないことを確認する条件をつけましょう。

//list[leader_txn][リーダー選出後のトランザクション]{
#@maprange(../code/chapter3/leader_txn/leader_txn.go,txn)
RETRY:
    select {
    case <-s.Done():
        log.Fatal("session has been orphaned")
    default:
    }
    err = e.Campaign(context.TODO(), name)
    if err != nil {
        log.Fatal(err)
    }
    leaderKey := e.Key()
    resp, err := s.Client().Txn(context.TODO()).
        If(clientv3util.KeyExists(leaderKey)).
        Then(clientv3.OpPut("/chapter3/leader_txn_value", "value")).
        Commit()
    if err != nil {
        log.Fatal(err)
    }
    if !resp.Succeeded {
        goto RETRY
    }
#@end
//}

上述したように、リーダーに選出された後も様々な理由でリーダーではなくなる可能性があります。
そこで自身がリーダーでなくなったことを検出したくなると思います。

//list[leader_watch][リーダーチェック]{
#@maprange(../code/chapter3/leader_watch/leader_watch.go,watch)
func watchLeader(ctx context.Context, s *concurrency.Session, leaderKey string) error {
    ch := s.Client().Watch(ctx, leaderKey, clientv3.WithFilterPut())
    for {
        select {
        case <-s.Done():
            return errors.New("session is closed")
        case resp, ok := <-ch:
            if !ok {
                return errors.New("watch is closed")
            }
            if resp.Err() != nil {
                return resp.Err()
            }
            for _, ev := range resp.Events {
                if ev.Type == clientv3.EventTypeDelete {
                    return errors.New("leader key is deleted")
                }
            }
        }
    }
}

#@end
//}

この関数はetcdと通信ができずにセッションが切れた場合や、リーダーキーが削除された場合、contextによって処理が中断された場合にエラーを返します。
エラーが返ってきたらこのプロセスはリーダーではなくなったということなので、再度リーダー選出からやり直したり、プログラムを終了させるなど適切な対応をおこないましょう。
