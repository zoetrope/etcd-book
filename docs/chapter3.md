# etcdによる分散処理プログラミング

前章ではGo言語を利用してetcdの基本的なデータの読み書きなどの操作をおこないました。
本章ではトランザクション処理や、分散システムを開発する上で必要となる分散ロックやリーダー選出のプログラミング方法を紹介していきます。

## Transaction

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter4/01_conflict/conflict.go

etcdにアクセスするクライアントが常に1つしか存在しないのであれば何も問題はありません。
しかし、現実には複数のクライアントが同時にetcdにデータを書き込んだり読み込んだりします。
このような場合には正しくデータの読み書きをしないと、データの不整合が発生する可能性があります。
例えば以下のような例をみてみましょう。

etcdから現在の値を読み取り、そこに引数で指定した値を追加して保存するという単純な`addValue`という関数を用意します。

```go
func addValue(client *clientv3.Client, key string, d int) {
	resp, _ := client.Get(context.TODO(), key)
	value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
	value += d
	client.Put(context.TODO(), key, strconv.Itoa(value))
}
```

`/chapter4/conflict`というキーに10をセットします。
その後に、先程の`addValue()`関数に5と-3を渡してgoroutineとして並列に実行します。

```go
key := "/chapter4/conflict"
client.Put(context.TODO(), key, "10")
go addValue(client, key, 5)
go addValue(client, key, -3)
time.Sleep(1 * time.Second)
resp, _ := client.Get(context.TODO(), key)
fmt.Println(string(resp.Kvs[0].Value))
```

このコードの実行結果は12になってほしいところです。
しかし実際に実行してみると、結果は15になったり7になったりばらつきます。
片方のgoroutineで値を読み込んでから書き込むまでの間に、もう一方のgoroutineが値を書き換えてしまっているためにこのような問題が発生します。

ではこの`addValue()`関数を、トランザクションを利用して問題が起きないように書き換えてみましょう。

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter4/02_transaction/transaction.go

```go
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
```

etcdにおけるトランザクションは楽観ロック方式です。
すなわちデータに対してのロックはせず、データを取得してから更新するまでの間に変更がなければデータを更新し、そうでなければデータを更新しないという方式です。

このコードを実行すると結果は必ず12になり、期待する結果が得られます。
最初のコードと比べると複雑になっていますが、順に解説していきます。

まず`client.Get()`で値を取得し、このときの`ModRevision`を保持しておきます。

つぎに`client.Txn()`メソッドを利用してトランザクションを開始します。
`If(clientv3.Compare(clientv3.ModRevision(key), "=", rev))`では、指定したキーの現在の`ModRevision`と、最初に値を取得したときの`ModRevision`を比較しています。
すなわち、値を取得したときから現在までの間に値が書き換えられていないかどうかをチェックしています。

この`If`の条件が成立した場合に`Then`で指定した処理が実行されます。
ここでは`clientv3.OpPut()`を利用してデータを書き込んでいます。
最後に`Commit()`でトランザクションをコミットします。

トランザクションの`If`の条件が成立しなくてもエラーにはなりません。
`If`の条件が成立したかどうかは、レスポンスの`Succeeded`で判断することができます。
ここでは`If`の条件が成立しなかった、すなわち最初にデータを取得したときと書き込むときのリビジョンが異なり、データが書き込めなかった場合に`goto`を使って最初から処理をやり直しています。

### トランザクションの記法

etcdのトランザクションは、基本的に次のような順序でメソッドを呼び出して利用します。

```go
tresp, err := client.Txn(context.TODO()).
	If(・・・).
	Then(・・・).
	Else(・・・).
	Commit()
```

このとき、`If`、`Then`、`Else`はそれぞれ省略することができます。
ただし2回以上記述することはできません。
また、`Then`の後に`If`を呼び出したり、`Else`の後に`Then`を呼び出すなど順序を入れ替えることもできません。

`If`は条件を複数指定することができます。
指定した条件はAND条件となります。

```go
tresp, err := client.Txn(context.TODO()).
	If(
		clientv3.Compare(clientv3.ModRevision(key1), "=", rev1),
		clientv3.Compare(clientv3.ModRevision(key2), "=", rev2),
	).
	Then(・・・).
	Commit()
```

`Then`や`Else`で実行する処理を複数指定することもできます。
次のように`Put`と`Delete`を1つのトランザクション内で同時に実行することができます。

```go
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
```

このとき、`Put`や`Delete`の結果は`tresp.Responses`にスライスとして入っています。
`GetResponsePut()`や`GetResponseDeleteRange()`を利用すると、
それぞれのオペレーションに応じたレスポンスの型にキャストすることができます。

オペレーションは、`clientv3.OpGet()`、`clientv3.OpPut()`、`clientv3.OpDelete()`、`clientv3.OpTxn()`、`clientv3.OpCompact()`が利用できます。
`clientv3.OpTxn()`を利用すれば、ネストした複雑なトランザクションを記述することも可能です。

### いろいろなIf条件

前述の例では`If(clientv3.Compare(clientv3.ModRevision(key), "=", rev))`のように、ModRevisionを比較していました。
これ以外にも`clientv3.Value()`で値の比較、`clientv3.Version()`でバージョンの比較、`clientv3.CreateRevision()`でCreateRevisionの比較をすることが可能です。

また比較演算子は`"="`の他に`"!="`、`"<"`、`">"`が利用できます。

`If(clientv3util.KeyExists(key))`や`If(clientv3util.KeyMissing(key))`を利用すれば、キーの有無によって条件分岐をすることが可能です。
なお、これらを利用するためには`"go.etcd.io/etcd/clientv3/clientv3util"`パッケージをインポートする必要があります。

## Concurrency

etcdでは分散システムを開発するために便利な機能をconcurrencyパッケージとして提供しています。
ここでは、Mutex、STM(Software Transactional Memory)、Leader Electionという3つの機能を紹介します。
これらを利用するためには`"go.etcd.io/etcd/clientv3/concurrency"`パッケージをインポートする必要があります。

### Session

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter4/03_session/session.go

concurrencyパッケージの各機能を解説する前に、Session機能について紹介しておきましょう。

後に解説するMutexでは、ロック済みかどうかを表すためにキーバリューを利用します。
もしロックを取得したプロセスがロックを解除しないまま終了してしまったら、非常に困ったことになってしまうでしょう。

Sessionを利用すると、作成したキーにリース期間が設定されるため、プロセスが不意に終了してもキーの有効期限が過ぎたらロックは解除されます。
また、プロセスが生きている間はリース期間を更新し続けるようにもなっています。

Sessionは以下のように作成することができます。

```go
session, err := concurrency.NewSession(client)
```

なお、デフォルトでのリース期間は60秒に設定されています。
この時間を変更したい場合は、次のように`concurrency.WithTTL()`を利用することができます。

```go
session, err := concurrency.NewSession(client, concurrency.WithTTL(180))
```

また、ロックを取得したプロセスが何か処理をしているときに、etcdとの接続が切れてしまっていたりリースを失効していた場合は、処理を中止すべきでしょう。
Sessionの`Done()`メソッドを利用すれば、セッションが切れた通知を受け取ることが可能です。

```go
select {
case <-session.Done():
	log.Fatal("session has been orphaned")
}
```

### Mutex

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter4/04_mutex/mutex.go

Mutexは排他制御を実現するための機構です。

一般的なMutexは異なるスレッド間やプロセス間での排他制御を実現するために利用されます。
etcdの提供するMutexでは異なるサーバ上のプロセス間での排他制御が可能になります。

```go
session, err := concurrency.NewSession(client)
if err != nil {
	log.Fatal(err)
}
mutex := concurrency.NewMutex(session, "/chapter4/mutex")
err = mutex.Lock(context.TODO())
if err != nil {
	log.Fatal(err)
}
fmt.Println("acquired lock")
time.Sleep(5 * time.Second)
err = mutex.Unlock(context.TODO())
if err != nil {
	log.Fatal(err)
}
fmt.Println("released lock")
```

Mutexは、前節で解説したSessionと、ロック用のキープレフィックスを指定して作成します。
ここで指定したキープレフィックスをもとに排他制御されるので、複数のプロセス間で同じキープレフィックスを指定する必要があります。

```go
mutex := concurrency.NewMutex(session, "/chapter4/mutex")
```

作成したMutexを利用してロックを取得します。

```go
err = mutex.Lock(context.TODO())
```

このときロックが取得できれば呼び出しは成功しますが、すでに他のプロセスがロックを取得済みだった場合は、ロックが取得できるまでブロックされます。
ロックが取得できなかったときにタイムアウトさせたい場合は、タイムアウトを設定した`context`を渡します。

ロックが必要な処理が終わったらロックを解放します。
解放漏れを防ぐためにも、`defer`を利用してスコープを抜けたときに必ずロックを解放するのがおすすめです。

```go
err = mutex.Unlock(context.TODO())
```

このコードを複数個実行してみましょう。
いずれかのプロセスがロックを取得している間は、他のプロセスがロック取得待ちになることを確認できます。

また、ロック取得中のプロセスをCtrl+Cで強制終了してみるとどうでしょうか？
その場合はリース期間(デフォルトでは60秒)経過するまでは、いずれのプロセスもロックを取得できずに待ち状態になるでしょう。


さて、ロックが取れたからといって安心してはいけません。
ロックした後に一度ネットワーク接続が切れていたり、Sessionのリース期間が終了しているかもしれません。
そうなってしまうと、プログラムは自分がロックしたつもりで動作しているのに、実際にはロックされていないという状況に陥ってしまいます。

そこで、ロックを取ったあとにetcdのキーバリューを操作する際には、トランザクションのIf条件に`Mutex.IsOwner()`を指定しましょう。

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter4/05_mutex_txn/mutex_txn.go

```go
	mutex := concurrency.NewMutex(session, "/chapter4/mutex_txn")
RETRY:
	select {
	case <-session.Done():
		log.Fatal("session has been orphaned")
	default:
	}
	err = mutex.Lock(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
	resp, err := client.Txn(context.TODO()).
		If(mutex.IsOwner()).
		Then(clientv3.OpPut("/chapter4/mutex_owner", "test")).
		Commit()
	if err != nil {
		log.Fatal(err)
	}
	if !resp.Succeeded {
		fmt.Println("the lock was not acquired")
		mutex.Unlock(context.TODO())
		goto RETRY
	}
	err = mutex.Unlock(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
```

### STM(Software Transactional Memory)

#### STMの基本的な使い方

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter4/06_stm/stm.go

先ほどはトランザクションを利用してデータの不整合が起きないような処理を記述しました。
しかし、リビジョンの比較やリトライ処理は少々記述が複雑でした。
扱うキーバリューが増えてくるとトランザクションの記述はさらに複雑になります。
そこで、etcdではSTM(Software Transactional Memory)という仕組みを提供し、簡単に排他制御を記述することが可能になっています。

では先程のトランザクション処理をSTMを使って書き換えてみましょう。

```go
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
```

`concurrency.NewSTM()`を利用して、第2引数で渡した関数の中でキーバリューを操作します。
このように記述すると、この関数のなかの処理がトランザクションとして実行されます。
Getしたキーの値がトランザクションが完了するまでの間に変更された場合は、自動的に処理がリトライされます。

#### トランザクション分離レベル

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter4/07_isolation/isolation.go

`concurrency.NewSTM()`に以下のようにオプションを指定することで、トランザクション分離レベルを指定することが可能です。

```go
_, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
	・・・
}, concurrency.WithIsolation(concurrency.RepeatableReads))
```

トランザクション分離レベルは以下のいずれかを指定することができます。
上のほうが分離レベルが高くデータの不整合が発生しにくくなりますが、その代わりに並列実行がしにくくなります。
基本的にはデフォルトの`SerializableSnapshot`を利用すればよいでしょう。

 * SerializableSnapshot (デフォルト)
 * Serializable
 * RepeatableReads
 * ReadCommitted

分離レベルが異なると値の読み取り方が変わります。
例えば、以下のように複数の値を読み取る場合、
SerializableSnapshotとSerializableでは、key1とkey2は必ず同じリビジョンの値が読まれます。
一方RepeatableReadsとReadCommittedでは、key1とkey2の値が異なるリビジョンから読み込まれる場合があります(ファントムリードと呼びます)。

```
_, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
	v1 := stm.Get(key1)
	v2 := stm.Get(key2)
		・・・
	return nil
})
```

:::message
一般的なトランザクション分離レベルのReadCommittedでは、ファジーリード(同じキーの値を複数回読んだときに異なる値が読まれる)が発生します。
しかし現在(v3.4.13)のetcdの実装では、一度読み込んだ値はキャッシュされているためファジーリードは発生しません。
:::

また、分離レベルが異なるとコンフリクトの検出方式も変わります。
以下のようにkey1の値を読み取って加工し、key2に書き込むような処理を考えてみます。

```go
_, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
	v1 := stm.Get(key1)
	v2 := translate(v1)
	stm.Put(key2, v2)
	return nil
})
```

SerializableSnapshotでは、key1を読み取ってからトランザクションが完了するまでの間に、key1とkey2の値が更新されていたらコンフリクトとみなします。
すなわち、読み取りに利用したキーと書き込みに利用したキーの両方をコンフリクトの判定に使います。

RepeatableReadsとSerializableは、key1が更新されていたらコンフリクトになります。
すなわち、読み取りに利用したキーのみをコンフリクトの判定に使います。

ReadCommittedはまったくコンフリクトを検出しません。通常は利用しないほうがよいでしょう。

### Leader Election

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter4/08_leader/leader.go

次にLeader Election機能について紹介します。
これは、異なるサーバー上で動作する複数のプロセスの中から、1つのプロセスをリーダーとして選出するための仕組みです。

```go
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
e := concurrency.NewElection(s, "/chapter4/leader")

err = e.Campaign(context.TODO(), name)
if err != nil {
	log.Fatal(err)
}
for i := 0; i < 10; i++ {
	fmt.Println(name + " is a leader.")
	time.Sleep(3 * time.Second)
}
err = e.Resign(context.TODO())
if err != nil {
	log.Fatal(err)
}
```

上記のプログラムを複数個立ち上げてください。
いずれか1つだけのプロセスがリーダーとして実行されていることが確認できると思います。
また、リーダーが処理を完了すると、別のプロセスが新たなリーダーとして処理を実行することが確認できるでしょう。

```
$ go run leader.go test1
test1 is a leader.
test1 is a leader.
test1 is a leader.
test1 is a leader.
test1 is a leader.
```

ではここで、リーダーが明示的に`Resign`せずに終了してしまったらどうなるでしょうか？
Ctrl+Cを押してリーダーのプロセスを途中で終了させてみてください。
他のプロセスがリーダーになることはなく、その状態で60秒ほど待つとようやく他のプロセスがリーダーになります。

これはLease機能を利用して、リーダーキーのリース期間が延長されない場合は自動的にキーが削除されるようになっています。
リース期間はデフォルトで60秒に設定されていますが、この値を変更したい場合はsessionを作成する際に`concurrency.NewSession(client, concurrency.WithTTL(30))`のようにTTLを変更できます。

#### リーダーによるトランザクション処理

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter4/09_leader_txn/leader_txn.go

リーダーになったからといって安心してはいけません。
ネットワーク障害が発生したりして、リーダーキーのリース期間が終了しているかもしれません。
自分がリーダーだと思って行動していたのに実はリーダーではなかったという状況に陥ります。
そこで、トランザクションのIf条件にリーダーキーが消えていないことを確認する条件をつけましょう。

```go
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
		Then(clientv3.OpPut("/chapter4/leader_txn_value", "value")).
		Commit()
	if err != nil {
		log.Fatal(err)
	}
	if !resp.Succeeded {
		goto RETRY
	}
```

#### リーダーの監視

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter4/10_leader_watch/leader_watch.go

上述したように、リーダーに選出された後も様々な理由でリーダーではなくなる可能性があります。
そこで自身がリーダーでなくなったことを検出したくなると思います。

```go
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
```

この関数はetcdと通信ができずにセッションが切れた場合や、リーダーキーが削除された場合、contextによって処理が中断された場合にエラーを返します。
エラーが返ってきたらこのプロセスはリーダーではなくなったということなので、再度リーダー選出からやり直したり、プログラムを終了させるなどの対応をおこないます。

このプログラムを起動した後、リーダーのキーを削除してみましょう。

```
$ etcdctl del /chapter4/leader_watch/694d74c35cb3a702
```

以下のようにリーダーキーが削除されたことを検出し、プログラムが終了します。

```
$ go run leader_watch.go test
leader key: /chapter4/leader_watch/694d74c35cb3a702
2020-09-25 15:46:06.272644 I | leader key is deleted
exit status 1
```
