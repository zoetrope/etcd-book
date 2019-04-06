= etcdによる分散処理プログラミング

前章ではGo言語を利用してetcdの基本的なデータの読み書きなどの操作をおこないました。
本章ではトランザクション処理や、分散システムを開発する上で必要となる分散ロックやリーダー選出のプログラミング方法を紹介していきます。

== Transaction

etcdにアクセスするクライアントが常に1つしか存在しないのであれば何も問題はありません。
しかし、現実には複数のクライアントが同時にetcdにデータを書き込んだり読み込んだりします。
このようなとき、正しくデータの読み書きをおこなわないと、データの不整合が発生する可能性があります。
例えば以下のような例をみてみましょう。

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

etcdにおけるトランザクションは楽観ロック方式です。
すなわち、データに対してのロックはおこなわず、データ取得時から更新時の間に変更がおこなわれていなければデータを更新し、
そうでなければデータを更新しないという方式です。

このコードを実行すると結果は必ず12になり、期待する結果が得られます。
最初のコードと比べると複雑になっていますが、順に解説していきます。

まず@<code>{Get()}で値を取得し、このときの@<code>{ModRevision}を保持しておきます。

つぎに@<code>{client.Txn()}メソッドを利用してトランザクションを開始します。
@<code>{If(clientv3.Compare(clientv3.ModRevision(key), "=", rev))}では、
指定したキーの現在の@<code>{ModRevision}と、最初に値を取得したときの@<code>{ModRevision}を比較しています。
すなわち、値を取得したときから現在までの間に値が書き換えられていないかどうかをチェックしています。

このIfの条件が成立した場合にThenで指定した処理が実行されます。
ここでは@<code>{clientv3.OpPut()}を利用してデータの書き込みをおこなっています。
最後に@<code>{Commit()}でトランザクションを完了します。

トランザクション後に@<code>{tresp.Succeeded}をチェックしています。
この値はIfの条件が成立した場合にtrueになります。
ここでは、Ifの条件が成立しなかった、すなわちデータの書き込みがおこなえなかった場合に、
最初から処理をやり直すようにしています。

=== トランザクションの記法

etcdのトランザクションは、基本的に次のような順序でメソッドを呼び出して利用します。

//list[?][]{
tresp, err := client.Txn(context.TODO()).
    If(・・・).
    Then(・・・).
    Else(・・・).
    Commit()
//}

このとき、If, Then, Elseはそれぞれ省略することができます。ただし2回以上記述することはできません。
また、Thenの後にIfを呼び出したり、Elseの後にThenを呼び出すなど順序を入れ替えることもできません。

Ifは条件を複数指定することができます。指定した条件はAND条件となります。

//list[?][]{
tresp, err := client.Txn(context.TODO()).
    If(
        clientv3.Compare(clientv3.ModRevision(key1), "=", rev1),
        clientv3.Compare(clientv3.ModRevision(key2), "=", rev2),
    ).
    Then(・・・).
    Commit()
//}

ThenやElseで実行する処理を複数指定することもできます。
次のようにPutとDeletを1つのトランザクション内で同時に実行することができます。

//list[?][]{
tresp, err := client.Txn(context.TODO()).
    Then(
        clientv3.OpPut(key1, value1),
        clientv3.OpDelete(key2),
    ).
    Commit()
if err != nil {
    return err
}
presp := tresp.Responses[0].GetResponsePut()
dresp := tresp.Responses[1].GetResponseDeleteRange()
//}

このとき、PutやDeleteの結果は@<code>{tresp.Responses}にスライスとして入っています。
@<code>{GetResponsePut()}や@<code>{GetResponseDeleteRange()}などを利用すると、
それぞれのオペレーションに応じたレスポンスの型にキャストすることができます。

オペレーションは、@<code>{clientv3.OpGet()}、@<code>{clientv3.OpPut()}、@<code>{clientv3.OpDelete()}、
@<code>{clientv3.OpTxn()}などが利用できます。
@<code>{clientv3.OpTxn()}を利用すれば、ネストした複雑なトランザクションを記述することも可能です。

=== いろいろなIf条件

前述の例では@<code>{If(clientv3.Compare(clientv3.ModRevision(key), "=", rev))}のように、
ModRevisionの比較をおこなっていました。
これ以外にも@<code>{clientv3.Value()}で値の比較、@<code>{clientv3.Version()}でバージョンの比較、
@<code>{clientv3.CreateRevision()}でCreateRevisionのリビジョンの比較などをおこなうことが可能です。

また比較演算子は@<code>{"="}の他に@<code>{"!="}、@<code>{"<"}、@<code>{">"}が利用できます。

また、@<code>{If(clientv3util.KeyExists(key))}や@<code>{If(clientv3util.KeyMissing(key))}を
利用すれば、キーの有無によって条件分岐をすることが可能です。
なお、これらを利用するためには@<code>{"github.com/coreos/etcd/clientv3/clientv3util"}パッケージをインポートする必要があります。

== Concurrency

etcdでは分散システムを開発するために便利な機能をconcurrencyパッケージとして提供しています。
ここでは、Mutex、STM(Software Transactional Memory)、Leader Electionという3つの機能を紹介します。

なお、これらを利用するためには@<code>{"github.com/coreos/etcd/clientv3/concurrency"}パッケージをインポートする必要があります。

=== Session

concurrencyパッケージの各機能を解説する前に、Session機能について紹介しておきましょう。

後に解説するMutexでは、ロック済みかどうかを表すためにキーバリューを利用します。
もしロックを取得したプロセスがロックを解除しないまま終了してしまったら、非常に困ったことになってしまうでしょう。
Sessionを利用すると、作成したキーにリース期間が設定されるため、プロセスが不意に終了してもキーの有効期限が過ぎたらロックは解除されます。
また、プロセスが生きている間はリース期間を更新し続けるようにもなっています。

Sessionは以下のように作成することができます。

//list[?][]{
session, err := concurrency.NewSession(client)
//}

なお、デフォルトでのリース期間は60秒に設定されています。
この時間を変更したい場合は、次のように@<code>{concurrency.WithTTL()}を利用することができます。

//list[?][]{
session, err := concurrency.NewSession(client, concurrency.WithTTL(180))
//}

また、ロックを取得したプロセスが何か処理をしているときに、
etcdとの接続が切れてしまっていたりリースを失効していた場合は、即座に処理を中止すべきでしょう。
Sessionの@<code>{Done()}メソッドを利用すれば、セッションが切れた通知を受け取ることが可能です。

//list[?][]{
select {
case <-session.Done():
    log.Fatal("session has been orphaned")
}
//}

=== Mutex

//list[?][]{
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
#@maprange(../code/chapter3/mutex_txn/mutex_txn.go,owner)
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
    if !resp.Succeeded {
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

@<hd>{chapter3|Transaction}ではトランザクションを利用してデータの不整合が起きないような処理を記述しました。
しかし、リビジョンの比較やリトライ処理は少々記述が難しかったのではないでしょうか。
扱うキーバリューが増えてくるとトランザクションの記述はさらに複雑になります。
そこで、etcdではSTM(Software Transactional Memory)という仕組みを提供し、簡単に排他制御を記述することが可能になっています。

では先程のトランザクション処理をSTMを使って書き換えてみましょう。

//list[?][]{
#@maprange(../code/chapter3/stm/stm.go,stm)
func addValue(client *clientv3.Client, key string, d int) {
    _, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
        v := stm.Get(key)
        value, _ := strconv.Atoi(v)
        value += d
        stm.Put(key, strconv.Itoa(value))
        return nil
    })
    if err != nil {
        log.Fatal(err)
    }
}

#@end
//}

@<code>{concurrency.NewSTM()}を利用して、第2引数で渡した関数の中でキーバリューの操作をおこないます。
このように記述すると、この関数のなかの処理がトランザクションとして実行されます。
Getしたキーの値がPutするまでの間に変更された場合はリトライ処理がおこなわれます。


また@<code>{concurrency.NewSTM()}では、以下のようにオプションを指定することで、
トランザクション分離レベルを指定することが可能です。

//list[?][]{
_, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
    ・・・
}, concurrency.WithIsolation(concurrency.RepeatableReads))
//}

トランザクション分離レベルは以下のいずれかを指定することができます。
上のほうが分離レベルが高くデータの不整合が発生しにくくなりますが、その代わりに並列実行がしにくくなります。
 基本的にはデフォルトの@<code>{SerializableSnapshot}を利用すればよいでしょう。

 * SerializableSnapshot (デフォルト)
 * Serializable
 * RepeatableReads
 * ReadCommitted

分離レベルが異なると値の読み取り方が変わります。
例えば、以下のように複数の値を読み取る場合、
SerializableSnapshotとSerializableでは、key1とkey2は必ず同じリビジョンの値が読まれます。
一方RepeatableReadsとReadCommittedでは、key1とkey2の値が異なるリビジョンから読み込まれる場合があります(ファントムリードと呼びます)。

//list[?][]{
_, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
    v1 := stm.Get(key1)
    v2 := stm.Get(key2)
        ・・・
    return nil
})
//}

また、分離レベルが異なるとコンフリクトの検出方式も変わります。
以下のようにkey1の値を読み取って加工し、key2に書き込むような処理を考えてみます。

//list[?][]{
_, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
    v1 := stm.Get(key1)
    v2 := translate(v1)
    stm.Put(key2, v2)
    return nil
})
//}

SerializableSnapshotでは、key1とkey2の
ReadCommittedはコンフリクトを検出しない。
RepeatableReadsとSerializableは、読み取った値が更新されていたらコンフリクト


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
