# 関連ツール

## gofail

[gofail](https://github.com/coreos/gofail)は、etcdの開発チームがつくったFailure Injectionのためのツールです。
Go言語で書かれたプログラム中に故意にエラーを発生させるポイント(failpoint)を埋め込み、任意のタイミングでプログラムの挙動を変えることができます。

チャプター3でWatchの際に取りこぼしを防ぐためのプログラムを紹介しました。

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

このコードでは`doSomething()`関数で何か重要な処理をおこない、`saveRev()`で処理が完了した値のリビジョンをファイルに書き出しています。
この`doSomething()`と`saveRev()`の間にプログラムがクラッシュしてしまうと、処理が完了した値のリビジョンとファイルに保存したリビジョンに不整合が発生してしまい、再度このプログラムを実行すると、すでに処理が完了したはずのデータが再度実行されてしまいます。

そんなタイミングでクラッシュするのか？と思われるかもしれませんが、人間がプロセスをkillしたり、OOMキラーによって停止させられたり、停電が発生することも考えられます。
今回の例ではファイルの書き出しをおこなっているので、ストレージが故障したり空き容量がない場合も考えられます。
信頼性の高いシステムを構築する場合には、様々な異常事態を想定してプログラムを実装する必要があります。

ところで、そのような想定をしたプログラムを実装したとしても、どのようにテストすればいいのでしょうか？

### gofailを使ってみよう

コード: https://github.com/zoetrope/etcd-programming/tree/master/chapter5/failpoint

gofail をインストールします。

```
$ go get -u github.com/etcd-io/gofail/...
```

プログラム中にfailpointを埋め込むためには、プログラム中に`gofail`というキーワードから始まるコメントを記述します。
ブロックコメント内に記述してもfailpointとして認識されないため、必ず行コメントで記述する必要があります。

また、mainパッケージ内にはfailpointを記述することはできません。

```go
rev := nextRev()
fmt.Printf("loaded revision: %d\n", rev)
ch := client.Watch(context.TODO(), "/chapter5/watch_file", clientv3.WithRev(rev))
for resp := range ch {
	if resp.Err() != nil {
		log.Fatal(resp.Err())
	}
	for _, ev := range resp.Events {
		doSomething(ev)
		// gofail: var ExampleOneLine struct{}
		err := saveRev(ev.Kv.ModRevision)
		if err != nil {
			log.Fatal(err)
		}
		fmt.Printf("saved: %d\n", ev.Kv.ModRevision)
	}
}
```

failpointを埋め込んでビルドするには、まず下記のコマンドを実行します。

```
$ cd ./chapter5/failpoint/
$ gofail enable
```

するとgofailから始まるコメントを記述した箇所が書き換えられ、`*.fail.go` というファイルが生成されます。
あとは生成されたコードを含めて通常通りにビルドします。

```
$ go build ./chapter5/failpoint
```

埋め込まれたfailpointは初期状態ではoffになっているので、実際にfailさせるためにはfailpointの有効化をおこなう必要があります。
failpointを有効化させる手段としては、プログラムの起動時に環境変数で指定するかHTTP APIで有効化するか2通りの方法がありますが、ここではHTTP APIを利用した方法を紹介します。

まずgofailのエンドポイントを環境変数で指定してプログラムを起動します。

```
GOFAIL_HTTP="127.0.0.1:1234" ./cmd
```

下記のAPIを叩くと、failpointの一覧を取得することができます。

```
$ curl http://127.0.0.1:1234/
github.com/zoetrope/etcd-programming/chapter5/failpoint/ExampleOneLine=
```

failpointを有効化するためには下記のAPIを実行します。
ここでは、`ExampleOneLine`の箇所で`panic`を発生させるように設定しています。

```
$ curl http://127.0.0.1:1234/github.com/zoetrope/etcd-programming/chapter5/failpoint/ExampleOneLine -XPUT -d'panic'
```

再度下記のAPIを実行すると、failpointが設定されていることが確認できます。

```
$ curl http://127.0.0.1:1234/
github.com/zoetrope/etcd-programming/chapter5/failpoint/ExampleOneLine=panic
```

ではetcdctlで何かデータを書き込んでみましょう。

```
$ etcdctl put /chapter5/watch_file value1
```

プログラムは、値を受け取った後にpanicで終了しました。

```
$ GOFAIL_HTTP="127.0.0.1:1234" ./cmd
loaded revision: 0
[123] PUT "/chapter5/watch_file" : "value1"
panic: failpoint panic: {}

goroutine 1 [running]:
github.com/etcd-io/gofail/runtime.actPanic(0xc000330140, 0x9a5701, 0xb7de20)
        /home/zoetro/go/src/github.com/zoetrope/etcd-programming/vendor/github.com/etcd-io/gofail/runtime/terms.go:318 +0xb7
github.com/etcd-io/gofail/runtime.(*term).do(...)
        /home/zoetro/go/src/github.com/zoetrope/etcd-programming/vendor/github.com/etcd-io/gofail/runtime/terms.go:290
github.com/etcd-io/gofail/runtime.(*terms).eval(0xc000330180, 0x0, 0x0, 0x0, 0x0)
        /home/zoetro/go/src/github.com/zoetrope/etcd-programming/vendor/github.com/etcd-io/gofail/runtime/terms.go:105 +0xde
github.com/etcd-io/gofail/runtime.(*Failpoint).Acquire(0xc0000de780, 0x0, 0x0, 0xadbc50, 0x14)
        /home/zoetro/go/src/github.com/zoetrope/etcd-programming/vendor/github.com/etcd-io/gofail/runtime/failpoint.go:52 +0xa5
github.com/zoetrope/etcd-programming/chapter5/failpoint.Run()
        /home/zoetro/go/src/github.com/zoetrope/etcd-programming/chapter5/failpoint/failpoint.go:67 +0x3f5
main.main()
        /home/zoetro/go/src/github.com/zoetrope/etcd-programming/chapter5/failpoint/cmd/main.go:8 +0x25
```

failpointを無効化するには下記のAPIを実行します。

```
$ curl http://127.0.0.1:1234/my/package/path/SomeFuncString -XDELETE
```

書き換えられたコードをもとに戻すには、以下のコマンドを実行します。
必ずもとに戻しておきましょう。

```
$ gofail disable
```

### failpointで実行可能なアクション

上記の例ではpanicを埋め込みましたが、それ以外にも下記のアクションを指定することができます。

 * off: failpointの無効化
 * return: 任意の値を返す(返すことのできる型は bool, int, string, struct{} のみ)
 * sleep: 一定時間待つ(待ち時間はDurationフォーマットで指定可能。intで指定した場合はミリ秒になる)
 * panic: panicを発生させる
 * break: デバッガによるbreakを発生させる
 * print: 標準出力に文字列を出力する

コメントの1行目にはトリガーとなる変数定義を記述し、2行目以降にはfailpointを有効化した時に実行されるコードを記述することができます。
複数行の処理を埋め込みたい場合は、空行をあけずにコメントを記述します。

例えば任意の値を返すようなfailpointを埋め込みたい場合は、下記のように1行目に返す値となる変数宣言、2行目にreturnを記述します。

```go
func someFunc() string {
	// gofail: var SomeFuncString string
	// return SomeFuncString
	return "default"
}
```

なお、failpointとなる変数の型は `bool`, `int`, `string`, `struct{}` のみになります。

fail発生後に任意の処理を実行する必要がない場合(panicを発生させる場合など)は、以下のように `struct {}` 型の変数宣言のみを記述します。

```go
func ExampleOneLineFunc() string {
	// gofail: var ExampleOneLine struct{}
	return "abc"
}
```

failpointを有効化した時にgotoやラベル付きcontinue文を実行したい場合は、下記のように gofail の後ろにラベル名を記述することもできます。

```go
func ExampleRetry() {
	// gofail: RETRY:
	for i := 0; ; i++ {
		fmt.Println(i)
		time.Sleep(time.Second)
		// gofail: var RetryLabel struct{}
		// goto RETRY
	}
}
```

また、failを発生させる回数や確率を指定することもできます。

下記の例では、failpointが呼び出された最初の3回は`hello`という文字列を返し、その後は通常の状態に戻ります。

```
$ curl http://127.0.0.1:1234/my/package/path/SomeFuncString -XPUT -d'3*return("hello")'
```

下記の例では、50%の確率で`hello`を返すようになります。

```
$ curl http://127.0.0.1:1234/my/package/path/SomeFuncString -XPUT -d'50.0%return("hello")'
```
