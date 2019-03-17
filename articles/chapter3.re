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
            log.Fatal(err)
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

 * 複数の処理を同時実行するだけ
 * ネストしたトランザクション
 * Else

== Concurrency

利用するためには@<code>{"github.com/coreos/etcd/clientv3/concurrency"}パッケージをインポートする必要があります。


=== STM(Software Transactional Memory)

先程のトランザクション処理をSTMを使って書き換えてみましょう。

//listnum[stm][STM]{
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
     * 分離レベルをSerializable
     * さらに、最初に値を読み込んだとき
 * Serializable
     * 初回のGet時のrevを覚えておく。2回目以降のGetは他のキーの場合でも同じrevを使って読み込む。
 * RepeatableReads
 * ReadCommitted
     * 一般的なトランザクション分離レベルのRead Committedにはなっていない。
     * 一切トランザクションになっていないので使うべきではない。
     * ファジーリードも発生しない。一度readした値はキャッシュしているので必ず同じ値を返す。

//listnum[phantom][ファントムリード]{
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

=== Mutex

Mutex.IsOwner

== Declarative

kubectlは、kube-apiserverを経由してetcdにリソースを書き込みます。
kube-controller-managerは、kube-apiserverを経由してリソースの変更をWatchしている。
変更が合った時にetcdから取得したリソースと、実際に

kubeconのwatch apiの資料読む
