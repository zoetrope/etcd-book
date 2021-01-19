# etcdプログラミングの基本

本章ではetcdのクライアントライブラリを利用して、Go言語でetcdを操作するプログラムの書き方を紹介していきます。
ここではまず、etcdに接続するクライアントをの作り方と、それを利用したキー・バリューの読み書きの方法を解説します。
さらにキー・バリューの変更を監視する方法や、キーの有効期限を指定する方法を紹介します。

## クライアントを用意する

それではさっそくGo言語によるetcdプログラミングをはじめましょう。

### Go言語のセットアップ

Go言語の開発環境はセットアップ済みですか？
もしセットアップが完了していないのであれば https://golang.org/doc/install を参考にGoのセットアップをおこなってください。

Goのバージョンを確認しておきましょう。

```
$ go version
go version go1.15.2 linux/amd64
```

本書ではGo 1.15.2を利用します。
本書のコードは多少古いGoでも動くとは思いますが、最新版を利用することをおすすめします。

### クライアントプログラム

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter3/01_client/client.go

ではetcdにアクセスするプログラムを書いてみましょう。
以下のようなファイルを作成し、client.goという名前で保存してください。

```go
package main

import (
	"context"
	"fmt"
	"log"
	"time"

	"go.etcd.io/etcd/clientv3"
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

```

### go module

etcdは、v3.4からgo moduleに対応しました。
しかし、パッケージの切り方を間違えているため、タグ名を指定してクライアントライブラリを利用することができないという問題があります[^1]。
そこで、タグ名ではなくハッシュ値を指定する必要があります。

また、etcdはgrpc-goの古いAPIに依存しているため、grpc-go v1.30.0以降は利用できません[^2]。古いバージョンを指定しておく必要があります。

`go mod init`コマンドを実行した後、`go.mod`ファイルに以下の記述を追加してください。

```
require (
	go.etcd.io/etcd v0.0.0-20200824191128-ae9734ed278b
	google.golang.org/grpc v1.29.1
)
```

なお、これらの問題はv3.5以降で修正されるようです[^2][^3]。

[^1]: https://github.com/etcd-io/etcd/issues/12109
[^2]: https://github.com/etcd-io/etcd/issues/12124
[^3]: https://github.com/etcd-io/etcd/issues/11820

### コードの実行

etcdは起動していますか？
もし起動していないようなら前のチャプターを参考に起動してください。

etcdが起動しているコンピュータ上で、コンパイルしたプログラムを実行してみましょう。
以下のようなメッセージが表示されれば成功です。
このプログラムでは、etcdクラスタを構成するメンバーの情報を表示しています。

```
$ go run client.go
ID:10276657743932975437 name:"etcd-1" peerURLs:"http://localhost:2380" clientURLs:"http://0.0.0.0:2379"
```

もしetcdが起動していなかったり、接続するアドレスが間違っていたりすると、以下のようなエラーが発生します。
etcdが起動しているかどうか、プログラムが間違っていないかどうかを確認してください。

```
$ go run client.go
2019/03/14 20:34:02 dial tcp 127.0.0.1:2379: connect: connection refused
```

### ソースコード解説

まず`go.etcd.io/etcd/clientv3`をインポートしています。
これによりetcdのクライアントライブラリが利用できるようになります。

```go
import (
	"go.etcd.io/etcd/clientv3"
)
```

:::message
etcd v3.3までのパッケージ名は`github.com/coreos/etcd/clientv3`でしたが、etcd v3.4から`go.etcd.io/etcd/clientv3`に変わりました。さらにv3.5では`go.etcd.io/etcd/v3/clientv3`に変わるので注意してください。
:::

次に`main`関数の中を見ていきましょう。
`clientv3.Config`型の変数を用意しています。
これはetcdに接続するときに利用する設定で、接続先のアドレスや接続時のタイムアウト時間などを指定できます。
ここではlocalhost上の2379番ポートに接続し、接続タイムアウト時間が3秒になるように設定しています。

```go
cfg := clientv3.Config{
	Endpoints:   []string{"http://localhost:2379"},
	DialTimeout: 3 * time.Second,
}
```

この設定を利用して、`clientv3.New()`でクライアントを作成します。
このとき、etcdとの接続に失敗するとエラーが返ってくるのでエラー処理をしましょう。
また、etcdとのやり取りが終了したら`client.Close()`を呼び出すようにしておきます。

```go
client, err := clientv3.New(cfg)
if err != nil {
	log.Fatal(err)
}
defer client.Close()
```

最後に作成したクライアントを利用して、`MemberList()`を呼び出し、その結果を画面に表示しています。
このとき引数に`context.TODO()`を渡していますが、`context`については後ほど詳しく解説します。

```go
resp, err := client.MemberList(context.TODO())
if err != nil {
	log.Fatal(err)
}
for _, m := range resp.Members {
	fmt.Printf("%s\n", m.String())
}
```

このように、クライアントを作成しそれを利用して必要なAPIを呼び出すのが、etcdプログラミングの基本的な流れになります。

## Key Value

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter3/02_kv/kv.go

では、先ほど作成したクライアントを利用して、etcdにデータを書き込んでみましょう。

先ほどのクライアントを利用したコードをコピーし、`MemberList()`を呼び出す代わりに以下のようなコードを書いてみます。

```go
_, err = client.Put(context.TODO(), "/chapter3/kv", "my-value")
if err != nil {
	log.Fatal(err)
}
```

これを実行すると`/chapter3/kv`というキーに`my-value`という値が書き込まれます。
なお、`Put`を利用すると、指定したキーがなければ新しくデータが作成され、キーが存在していればその値が更新されることになります。

では、書き込んだ値が正しく読み込めるかどうか試してみましょう。
先程のコードの続きに以下の処理を追加します。

```go
resp, err := client.Get(context.TODO(), "/chapter3/kv")
if err != nil {
	log.Fatal(err)
}
if resp.Count == 0 {
	log.Fatal("/chapter3/kv not found")
}
fmt.Println(string(resp.Kvs[0].Value))
```

このコードを実行すると、書き込んだ値が読み込まれて`my-value`が表示されることでしょう。
もし書き込みや読み込みが失敗した場合は何らかのエラーメッセージが表示されます。

次に書き込んだ値を削除してみましょう。

```go
_, err = client.Delete(context.TODO(), "/chapter3/kv")
if err != nil {
	log.Fatal(err)
}
```

値を削除した後に読み込もうとしても、キーが見つからないという結果になるでしょう。

## Option

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter3/03_opts/opts.go

先ほど紹介した`Put()`, `Get()`, `Delete()`メソッドにはいろいろなオプションを指定することができます。
ここでは`client.Get()`を例にいくつかのオプションをみてみましょう。

```go
client.Put(context.TODO(), "/chapter3/option/key3", "val2")
client.Put(context.TODO(), "/chapter3/option/key1", "val3")
client.Put(context.TODO(), "/chapter3/option/key2", "val1")
resp, err := client.Get(context.TODO(), "/chapter3/option/",
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
```

このコードの実行結果は次のようになります。

```
/chapter3/option/key2:
/chapter3/option/key3:
/chapter3/option/key1:
```

まず`WithPrefix()`を指定すると、キーに指定したプレフィックス(ここでは`/chapter3/option`)から始まるキーをすべて取得します。
次に`WithSort()`を指定すると結果をソートすることができます。ここではValueの昇順で並び替えています。
最後の`WithKeysOnly()`を指定するとキーのみを取得します。そのため結果に値が出力されていません。

この他にもキーの数だけを返す`WithCountOnly`や、見つかった最初のキーだけを返す`WithFirstKey`などたくさんのオプションがあります。
ぜひ https://pkg.go.dev/github.com/coreos/etcd/clientv3#OpOption を一度みてください。

## MVCC (MultiVersion Concurrency Control)

etcdはMVCC(MultiVersion Concurrency Control)モデルを採用したキーバリューストアです。
すなわち、etcdに対して実施した変更はすべて履歴が保存されており、それぞれにリビジョン番号がつけられています。
リビジョン番号を指定して値を読み出せば、過去の時点での値も読み出すことができます。

### RevisionとVersion

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter3/04_revision/revision.go

では、具体的にリビジョン番号が更新されていく様子を見てみましょう。

その前に`Get()`メソッドのレスポンスを詳しく表示するヘルパー関数を用意しておきます。

```go
func printResponse(resp *clientv3.GetResponse) {
	fmt.Println("header: " + resp.Header.String())
	for i, kv := range resp.Kvs {
		fmt.Printf("kv[%d]: %s\n", i, kv.String())
	}
	fmt.Println()
}
```

では書き込んだ値を取得して、その結果を表示してみましょう。

```go
client.Put(context.TODO(), "/chapter3/rev/1", "123")
resp, _ := client.Get(context.TODO(), "/chapter3/rev", clientv3.WithPrefix())
printResponse(resp)
```

実行すると以下のように表示されます。

```
header: cluster_id:14841639068965178418 member_id:10276657743932975437 revision:41 raft_term:2
kv[0]: key:"/chapter3/rev/1" create_revision:41 mod_revision:41 version:1 value:"123"
```

`header`の中に`revision`というフィールドが見つかります。

* revision
    etcdのリビジョン番号。クラスタ全体で一つの値が利用される。etcdに何らかの変更(キーの追加、変更、削除)が加えられると値が1増える。

キーバリューの中には以下のフィールドが見つかります。

* create_revision
    このキーが作成されたときのリビジョン番号。
* mod_revision
    このキーの内容が最後に変更されたときのリビジョン番号。
* version
    このキーのバージョン。このキーに変更が加えられると値が1増える。

次に`/chapter3/rev/1`の値を更新してみましょう。

```go
client.Put(context.TODO(), "/chapter3/rev/1", "456")
resp, _ = client.Get(context.TODO(), "/chapter3/rev", clientv3.WithPrefix())
printResponse(resp)
```

`revision`, `mod_revision`, `version`の値が1つ増え、
`create_revision`はそのままの値になっています。

```
header: cluster_id:14841639068965178418 member_id:10276657743932975437 revision:42 raft_term:2
kv[0]: key:"/chapter3/rev/1" create_revision:41 mod_revision:42 version:2 value:"456"
```

次に別のキー`/chapter3/rev/2`に値を書き込んでみましょう。

```go
client.Put(context.TODO(), "/chapter3/rev/2", "999")
resp, _ = client.Get(context.TODO(), "/chapter3/rev", clientv3.WithPrefix())
printResponse(resp)
```

このときの情報を見ると`header`の`revision`は増加し、
`/chapter3/rev/1`の`mod_revision`, `version`, `create_revision`は値が変化していません。
一方、`/chapter3/rev/2`の`mod_revision`, `create_revision`は最新の`revision`になり、
`version`は1になっていることがわかります。

```
header: cluster_id:14841639068965178418 member_id:10276657743932975437 revision:43 raft_term:2
kv[0]: key:"/chapter3/rev/1" create_revision:41 mod_revision:42 version:2 value:"456"
kv[1]: key:"/chapter3/rev/2" create_revision:43 mod_revision:43 version:1 value:"999"
```

続いて`/chapter3/rev/1`が作成された時点でのリビジョン番号(41)を指定して値を読み出してみましょう。
リビジョンを指定して値を読み出すためには、`clientv3.WithRev()`を利用します。

```go
resp, _ = client.Get(context.TODO(), "/chapter3/rev",
	clientv3.WithPrefix(), clientv3.WithRev(resp.Kvs[0].CreateRevision))
printResponse(resp)
```

実行すると`/chapter3/rev/1`が作成されたときの値`123`が取得できています。
このリビジョンの時点では`/chapter3/rev/2`は存在しなかったため、その値は取得できませんでした。
またこのとき`header`の`revision`は、指定したリビジョンではなく最新のリビジョン番号が入っていることに注意しましょう。

```
header: cluster_id:14841639068965178418 member_id:10276657743932975437 revision:43 raft_term:2
kv[0]: key:"/chapter3/rev/1" create_revision:41 mod_revision:41 version:1 value:"123"
```

### コンパクション

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter3/05_compaction/compaction.go

etcdでは1つのキーに対して複数のリビジョンのデータが保存されていることが確認できました。
しかし、すべてのリビジョンを保存し続けているとディスク容量が逼迫してしまうため、適当なタイミングでコンパクションして古い履歴を削除するのが一般的です。

ではまず、1つのキーの値を何度か更新してみましょう。

```go
client.Put(context.TODO(), "/chapter3/compaction", "hoge")
client.Put(context.TODO(), "/chapter3/compaction", "fuga")
client.Put(context.TODO(), "/chapter3/compaction", "fuga")

resp, err := client.Get(context.TODO(), "/chapter3/compaction")
if err != nil {
	log.Fatal(err)
}

fmt.Println("--- prepared data: ")
for i := resp.Kvs[0].CreateRevision; i <= resp.Kvs[0].ModRevision; i++ {
	r, err := client.Get(context.TODO(), "/chapter3/compaction", clientv3.WithRev(i))
	if err != nil {
		log.Fatal(err)
	}
	fmt.Printf("rev: %d, value: %s\n", i, r.Kvs[0].Value)
}
```

実行すると、下記のようにリビジョンごとの値が保存されていることが分かります。

```
$ go run ./compaction.go
--- prepared data:
rev: 15, value: hoge
rev: 16, value: fuga
rev: 17, value: fuga
```

次に`Compact`関数を利用して、指定したリビジョンよりも古いデータをコンパクションします。

```go
_, err = client.Compact(context.TODO(), resp.Kvs[0].ModRevision)
if err != nil {
	log.Fatal(err)
}
fmt.Printf("--- compacted: %d\n", resp.Kvs[0].ModRevision)
for i := resp.Kvs[0].CreateRevision; i <= resp.Kvs[0].ModRevision; i++ {
	r, err := client.Get(context.TODO(), "/chapter3/compaction", clientv3.WithRev(i))
	if err != nil {
		fmt.Printf("failed to get: %v\n", err)
		continue
	}
	fmt.Printf("rev: %d, value: %s\n", i, r.Kvs[0].Value)
}
```

このコードを実行すると、指定したリビジョンよりも古いリビジョンのデータは取得できないことが確認できます。

```
$ go run ./compaction.go
--- compacted: 17
failed to get: etcdserver: mvcc: required revision has been compacted
failed to get: etcdserver: mvcc: required revision has been compacted
rev: 17, value: fuga
```

## context

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter3/06_timeout/timeout.go

etcdのクライアントが提供している多くのメソッドは、第一引数に`context.Context`を受けるようになっています。

`context.Context`は、Go言語においてブロッキング処理などの時間のかかる処理を、タイムアウトさせたりキャンセルさせるために利用される機構です。

etcdクライアントは、etcdサーバーと通信するため、 リクエストを投げてからどれくらいの時間で返ってくるかはわかりません。その間の処理はブロックされることになります。
そのため、処理を中断したいようなケースでは`context`を利用することになります。

それでは`context`を利用して処理をキャンセルする例をみてみましょう。

```go
ctx, cancel := context.WithTimeout(context.Background(), 1*time.Second)
defer cancel()
resp, err := client.Get(ctx, "/chapter3/timeout")
if err != nil {
	log.Fatal(err)
}
fmt.Println(resp)
```

この例では1秒でタイムアウトする`context`を作り、`client.Get()`の第一引数に渡しています。
これにより、`client.Get()`に時間がかかっていた場合、1秒経過すると途中で処理を終了します。
処理が途中で終了した場合、`client.Get()`はエラーを返します。

このプログラムを実行してみましょう。
通常はGetの処理に1秒もかからないため、問題なく値が取得できていると思います。

それでは`tc`コマンドを利用して、ネットワーク遅延を擬似的に発生させてみましょう。
なお、`tc`コマンドはLinux環境でしか利用できないのでご注意ください。
コンテナのveth名を取得するスクリプトは https://github.com/zoetrope/etcd-programming/blob/master/chapter3/06_timeout/veth.sh を利用してください。

```
$ VETH=$(sudo ./veth.sh etcd)
$ sudo tc qdisc add dev $VETH root netem delay 3s
```

もう一度プログラムを実行してみましょう。
少し待つと"context deadline exceeded"というメッセージが表示されてプログラムが終了するはずです。
このようなタイムアウト処理を利用すると、例えばネットワークの調子が悪いときに、処理を中断して管理者に通知を飛ばすような仕組みを構築することも可能になります。

遅延させたままでは困るので、もとに戻しておきましょう。

```
$ VETH=$(sudo ./veth.sh etcd)
$ sudo tc qdisc del dev $VETH root
```

なお本書のサンプルプログラムでは`context.TODO()`を利用していますが、実際にコードを書くときには適切なコンテキストを利用してください。

## Watch

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter3/07_watch/watch.go

etcdは、複数のプログラム間で情報を共有する目的で使われることがよくあります。
このとき、他のプログラムが情報を更新したことを定期的にチェックしていたのでは効率が悪くなってしまいます。
そこでetcdでは、キー・バリューの変更を通知するためにWatch APIを提供しています。

さっそく、Watch APIの使い方を見てみましょう。

```go
ch := client.Watch(context.TODO(), "/chapter3/watch/", clientv3.WithPrefix())
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
```

第2引数で監視対象のキーを指定し、第3引数以降でオプションを指定します。
ここでは`clientv3.WithPrefix()`オプションを指定してるため、`"/chapter3/watch/"`から始まるすべてのキーを監視対象としています。

```go
ch := client.Watch(context.TODO(), "/chapter3/watch/", clientv3.WithPrefix())
```

`Watch`はGo言語のchannelを返します。
for文でループを回せば、channelがクローズされるまで通知を受け続けることになります。
通知は`WatchResponse`型の変数として受け取ります。

```go
for resp := range ch {
	・・・
}
```

`WatchResponse`を受け取ったら、まずは`Err()`を呼び出してエラーチェックします。
etcdとの接続が切れた場合や、監視対象のキーがコンパクションで削除された場合にエラーが発生するので、対応が必要となります。

```go
if resp.Err() != nil {
	log.Fatal(resp.Err())
}
```

一つの`WatchResponse`には複数のイベントが含まれているので、ループを回して処理をおこないます。
イベントのタイプには"PUT"または"DELETE"が入っています。
さらにイベントタイプが"PUT"のときに、`IsCreate()`と`IsModify()`を利用することで、そのキーが新たに作成されたのか変更されたのかを判断することが可能です。

```go
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
```

では上記のコードを実行した状態で、etcdctlで`/chapter3/watch/`から始まるキーで値を書き込んでみましょう。

```
$ etcdctl put /chapter3/watch/key1 test
OK
$ etcdctl put /chapter3/watch/key1 foo
OK
$ etcdctl del /chapter3/watch/key1
1
```

下記のようにデータの作成、更新、削除のイベントを監視できていることが確認できます。

```
$ go run ./watch.go
CREATE "/chapter3/watch/key1" : "test"
MODIFY "/chapter3/watch/key1" : "foo"
DELETE "/chapter3/watch/key1" : ""
```

なお、イベントを受け取ったキーの以前の状態を知りたい場合もあるでしょう。
変更前の状態を取得するためには、Watchのオプションに`clientv3.WithPrevKV()`を指定します。
すると`ev.PrevKv`を利用して、変更前の値を取得することができます。

### 取りこぼしを防ぐ

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter3/08_watch_file/watch_file.go

Watch APIを呼び出すと、呼び出した時点からの変更が通知されます。
そのため、Watchを利用しているプログラムが停止している間にetcdに変更が加えられた場合、その変更を取りこぼすことになってしまいます。

Watch APIを呼び出すときに`clientv3.WithRev()`オプションを指定することで、特定のリビジョンからの変更をすべて受け取ることが可能になります。

ここではイベントを取りこぼさずに処理する例として、処理の完了したリビジョン番号をファイルに書き出しておき、プログラムを再度起動したときにはそのリビジョン番号から処理を再開するような実装を紹介します。

```go
rev := nextRev()
fmt.Printf("loaded revision: %d\n", rev)
ch := client.Watch(context.TODO(), "/chapter3/watch_file", clientv3.WithRev(rev))
for resp := range ch {
	if resp.Err() != nil {
		log.Fatal(resp.Err())
	}
	for _, ev := range resp.Events {
		doSomething(ev)
		err := saveRev(ev.Kv.ModRevision)
		if err != nil {
			log.Fatal(err)
		}
		fmt.Printf("saved: %d\n", ev.Kv.ModRevision)
	}
}
```

ファイルから次のリビジョンを読み込み、それをWatchのオプションとして`clientv3.WithRev()`で指定します。

```
rev := loadRev()
ch := client.Watch(context.TODO(), "/chapter3/watch_file", clientv3.WithRev(rev+1))
```

通知を受け取ったら、そのイベントを使ってなんらかの処理をおこない、そのときのリビジョン番号をファイルに保存します。

```go
doSomething(ev)
err := saveRev(ev.Kv.ModRevision)
```

リビジョン番号をファイルから読み取る処理を以下のように実装します。
ファイルに書き込まれているのは最後に処理したイベントのリビジョン番号なので、次のイベントから通知を開始するために+1した値を指定しています。
ファイルが見つからなかった場合や読み取りに失敗した場合は0を返すようにしています。
`clientv3.WithRev()`に0を指定した場合は、呼び出した時点からの変更が通知されることになります。

```go
func nextRev() int64 {
	p := "./last_revision"
	f, err := os.Open(p)
	if err != nil {
		os.Remove(p)
		return 0
	}
	defer f.Close()
	data, err := ioutil.ReadAll(f)
	if err != nil {
		os.Remove(p)
		return 0
	}
	rev, err := strconv.ParseInt(string(data), 10, 64)
	if err != nil {
		os.Remove(p)
		return 0
	}
	return rev + 1
}
```

リビジョン番号をファイルに書き出す処理は以下のように実装します。

```go
func saveRev(rev int64) error {
	p := "./last_revision"
	f, err := os.OpenFile(p, os.O_WRONLY|os.O_CREATE|os.O_TRUNC|os.O_SYNC, 0644)
	if err != nil {
		return err
	}
	defer f.Close()
	_, err = f.WriteString(strconv.FormatInt(rev, 10))
	return err
}
```

このプログラムを起動しetcdctlで値を書き込んでみましょう。

```
$ etcdctl put /chapter3/watch_file value1
OK
$ etcdctl put /chapter3/watch_file value2
OK
$ etcdctl put /chapter3/watch_file value3
OK
```

変更通知を受け取って処理をおこない、リビジョンをファイルに保存したことが確認できます。

```
$ go run watch_file.go
loaded revision: 0
[32] PUT "/chapter3/watch_file" : "value1"
saved: 32
[33] PUT "/chapter3/watch_file" : "value2"
saved: 33
[34] PUT "/chapter3/watch_file" : "value3"
saved: 34
```

つぎにこのプログラムを一旦終了し、その間にetcctlなどで値を変更してみましょう。

```
$ etcdctl put /chapter3/watch_file value4
OK
$ etcdctl put /chapter3/watch_file value5
OK
$ etcdctl put /chapter3/watch_file value6
OK
```

再度このプログラムを立ち上げてみます。

```
$ go run watch_file.go
loaded revision: 35
[35] PUT "/chapter3/watch_file" : "value4"
saved: 35
[36] PUT "/chapter3/watch_file" : "value5"
saved: 36
[37] PUT "/chapter3/watch_file" : "value6"
saved: 37
```

プログラムを終了していた間に発生した変更をすべて受け取って処理がおこなわれていることが見てとれます。

### Watchとコンパクション

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter3/09_watch_compact/watch_compact.go

リビジョンを指定してWatchを開始した場合、そのキーがすでにコンパクションされている可能性もあります。

`WatchResponse`の`CompactRevision`を利用すると、コンパクションされていない中で最も古いリビジョンが取得できるので、このリビジョンを使ってWatchを再開するなどの処理をおこないます。

```go
RETRY:
	fmt.Printf("watch from rev: %d\n", rev)
	ch := client.Watch(context.TODO(),
		"/chapter3/watch_compact", clientv3.WithRev(rev))
	for resp := range ch {
		if resp.Err() == rpctypes.ErrCompacted {
			rev = resp.CompactRevision
			goto RETRY
		} else if resp.Err() != nil {
			log.Fatal(resp.Err())
		}
		for _, ev := range resp.Events {
			fmt.Printf("%s %q : %q\n", ev.Type, ev.Kv.Key, ev.Kv.Value)
		}
	}
```

実行してみると、コンパクションされたリビジョンから読み直していることが確認できます。

```
$ go run watch_compact.go
watch from rev: 41
watch from rev: 43
PUT "/chapter3/watch_compact" : "5"
PUT "/chapter3/watch_compact" : "6"
PUT "/chapter3/watch_compact" : "7"
PUT "/chapter3/watch_compact" : "8"
PUT "/chapter3/watch_compact" : "9"
```

## Lease

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter3/10_lease/lease.go

etcdではLeaseという仕組みを利用して、キー・バリューに有効期限を指定することができます。
キーの登録時に有効期限を指定しておくと、その期限が過ぎたときに対象のキーは自動的に削除されます。

KubernetesではこのLease機能を利用してクラスタ内で発生したイベント情報を一時的に蓄えています。
またセッション情報を管理するために利用することも可能ですし、後述するMutexやLeader Electionでも利用されています。

ではLease機能の利用方法を見ていきましょう。

```go
lease, err := client.Grant(context.TODO(), 5)
if err != nil {
	log.Fatal(err)
}
_, err = client.Put(context.TODO(), "/chapter3/lease", "value",
	clientv3.WithLease(lease.ID))
if err != nil {
	log.Fatal(err)
}

for {
	resp, err := client.Get(context.TODO(), "/chapter3/lease")
	if err != nil {
		log.Fatal(err)
	}
	if resp.Count == 0 {
		fmt.Println("'/chapter3/lease' disappeared")
		break
	}
	fmt.Printf("[%v] %s\n",
		time.Now().Format("15:04:05"), resp.Kvs[0].Value)
	time.Sleep(1 * time.Second)
}
```

Lease機能を利用するためには、まずGrantを呼び出してLeaseを作成します。このとき有効期限を秒単位で指定します。

```go
lease, err := client.Grant(context.TODO(), 5)
```

そして、キー・バリューを書き込むときに`clientv3.WithLease()`オプションを指定します。

さて、このコードを実行してみましょう。
最初の5秒間は値が正しく取得できていますが、5秒後に値が取得できなくなったことがわかります。

```
$ ./lease
[21:40:59] value
[21:41:00] value
[21:41:01] value
[21:41:02] value
[21:41:03] value
[21:41:04] value
'/chapter3/lease' disappeared
```

有効期限を設定したキー・バリューでも、`KeepAliveOnce()`を呼び出すことでその有効期限を延長することができます。
`KeepAliveOnce()`の引数には、先ほど作成したleaseのIDを渡します。

```go
resp, err = client.KeepAliveOnce(context.TODO(), lease.ID)
if err != nil {
	log.Fatal(err)
}
```

`KeepAliveOnce()`を呼び出すと、有効期限が最初に指定した時間分だけ延長されます。
`KeepAliveOnce()`を周期的に呼び出すための仕組みとして、`KeepAlive()`があります。

```
_, err = client.KeepAlive(context.TODO(), lease.ID)
if err != nil {
	log.Fatal(err)
}
```

リースの期間を延長し続けるなんて、リースの意味がなくなってしまうのでは？と思うかもしれません。
しかし、この`KeepAlive()`は`KeepAliveOnce()`を周期的に呼び出しているだけです。
そのためプログラムが終了するとリースの期限は延長されなくなり、いずれそのキーは消えてしまいます。
この機能を利用することでプログラムの生存をチェックすることが可能になります。

また、指定した期限までまだ時間がある場合でも、そのキーを失効させたい場合があります。
その場合は、`Revoke()`を利用します。

```go
_, err = client.Revoke(context.TODO(), lease.ID)
if err != nil {
	log.Fatal(err)
}
```

## Namespace

コード: https://github.com/zoetrope/etcd-programming/blob/master/chapter3/11_namespace/namespace.go

チャプター2において、キーにはアプリケーションごとにプレフィックスをつけるのが一般的だと説明しました。
しかし、アプリケーションを開発する際にすべてのキーにプレフィックスを指定するのは少々めんどうではないですか？
そこで、etcdのクライアントライブラリではnamespaceという機能が提供されています。

キー・バリューの操作(Put, Get, Delete, Txnなど)に関してnamespaceを適用したい場合は、以下のように`namespace.NewKV()`関数を利用して新しいクライアントを作成します。

```go
newClient := namespace.NewKV(client.KV, "/chapter3")
```

この`newClient`を利用して`/ns/1`というキーに値を書き込んでみましょう。
すると実際には`/chapter3/ns/1`に値が書き込まれていることがわかります。

```go
newClient.Put(context.TODO(), "/ns/1", "hoge")
resp, _ := client.Get(context.TODO(), "/chapter3/ns/1")
fmt.Printf("%s: %s\n", resp.Kvs[0].Key, resp.Kvs[0].Value)
```

逆に`/chapter3`を省略して`/ns/2`というキーだけを指定して、`/chapter3/ns/2`に書き込まれた値を読み取ることができます。

```go
client.Put(context.TODO(), "/chapter3/ns/2", "test")
resp, _ = newClient.Get(context.TODO(), "/ns/2")
fmt.Printf("%s: %s\n", resp.Kvs[0].Key, resp.Kvs[0].Value)
```

また、`namespace.NewKV()`以外にも、namespaceをWatch関連の操作に適用する`NewWatcher()`とLease関連の操作に適用する`NewLease()`という関数もあります。

namespace適用後にキーバリュー操作用のクライアントやWatch用のクライアントを個別に管理するのは面倒です。
そこで、次のように既存のクライアントの機能を上書きしてしまうのがおすすめです。
このようにしておけば、`client`経由でキーバリューの操作をしたときもWatchしたときもnamespaceが適用されます。

```go
client.KV = namespace.NewKV(client.KV, "/chapter3")
client.Watcher = namespace.NewWatcher(client.Watcher, "/chapter3")
client.Lease = namespace.NewLease(client.Lease, "/chapter3")
```
