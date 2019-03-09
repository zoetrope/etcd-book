= etcdによる並列処理プログラミング

== Transaction


//listnum[tocttou][TOCTTOUの例]{
#@maprange(../code/chapter3/tocttou/tocttou.go,tocttou)
    addValue := func(d int) {
        resp, _ := client.Get(context.TODO(), "/chapter3/tocttou")
        value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
        value += d
        client.Put(context.TODO(), "/chapter3/tocttou", strconv.Itoa(value))
    }
    client.Put(context.TODO(), "/chapter3/tocttou", "10")
    go addValue(5)
    go addValue(-3)
    time.Sleep(1 * time.Second)
    resp, _ := client.Get(context.TODO(), "/chapter3/tocttou")
    fmt.Println(string(resp.Kvs[0].Value))
#@end
//}

このコードでは、最初に値に10をセットし5を足して3を引いたのですから、結果は12になってほしいところです。
しかし実際に実行してみると、結果は15になったり7になったりばらつきます。

このような問題をTOCTTOU(Time of check to time of use)と呼びます。

Transactionを利用したコードに書き換えてみましょう。

//listnum[txn][Transaction]{
#@maprange(../code/chapter3/transaction/transaction.go,txn)
    addValue := func(d int) {
    RETRY:
        resp, _ := client.Get(context.TODO(), "/chapter3/txn")
        rev := resp.Kvs[0].ModRevision
        value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
        value += d
        tresp, err := client.Txn(context.TODO()).
            If(clientv3.Compare(clientv3.ModRevision("/chapter3/txn"), "=", rev)).
            Then(clientv3.OpPut("/chapter3/txn", strconv.Itoa(value))).
            Else().
            Commit()
        if err != nil {
            return
        }
        if !tresp.Succeeded {
            goto RETRY
        }
    }
    client.Put(context.TODO(), "/chapter3/txn", "10")
    go addValue(5)
    go addValue(-3)
    time.Sleep(1 * time.Second)
    resp, _ := client.Get(context.TODO(), "/chapter3/txn")
    fmt.Println(string(resp.Kvs[0].Value))
#@end
//}

このコードを実行すると結果は必ず12になり、期待する結果が得られます。

@<code>{If(clientv3.Compare(clientv3.ModRevision("/chapter3/txn"), "=", rev))}では、現在の/chapter3/txnのModRevisionと、最初に値を取得したときのModRevisionを比較しています。
すなわち、値を取得したときと現在で/chapter3/txnの値が書き換えられていないかどうかをチェックしています。

このifの条件が成立するとThenで指定した処理が実行され、そうでなければElseの処理が実行されます。
ここではThenの中で値の書き込みをおこない、Elseのなかでは何もしていません。

そして最後にtresp.Succeededをチェックしています。
この値はIfの条件が成立した場合にtrueになります。

== ネストしたトランザクション


== Concurrency

MVCC

Mutex.IsOwner
