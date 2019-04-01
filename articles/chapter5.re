= ツール

== etcd-dump-logs

== etcd operator
 * minikube

@<href>{https://kubernetes.io/docs/tasks/tools/install-minikube/}
@<href>{https://kubernetes.io/docs/tasks/tools/install-kubectl/}

//terminal{
$ git clone https://github.com/coreos/etcd-operator.git
$ cd etcd-operator
$ example/rbac/create_role.sh
$ kubectl create -f example/deployment.yaml
$ kubectl create -f example/example-etcd-cluster.yaml

$ kubectl create -f example/example-etcd-cluster-nodeport-service.json
$ export ETCDCTL_API=3
$ export ETCDCTL_ENDPOINTS=$(minikube service example-etcd-cluster-client-service --url)
$ etcdctl put foo bar
//}

== Prometheus

=== PrometheusとGrafanaのセットアップ

まずはPrometheusのセットアップをおこないます。
Helmを利用すると簡単です。

下記のページを参考にしてHelmをセットアップしてください
https://github.com/helm/helm

//terminal{
$ helm init
//}

//terminal{
$ kubectl create namespace monitoring
//}

//terminal{
$ helm install --name prometheus --namespace monitoring stable/prometheus
//}

//cmd{
$ helm inspect values stable/grafana > grafana-values.yml
//}

//list[grafana-values][grafana-values.ymlの書き換え]{
--- grafana-values.yaml.org     2019-03-10 15:05:41.765498204 +0900
+++ grafana-values.yaml 2019-03-10 15:04:41.303469189 +0900
@@ -72,7 +72,7 @@
 ## ref: http://kubernetes.io/docs/user-guide/services/
 ##
 service:
-  type: ClusterIP
+  type: NodePort
   port: 80
   targetPort: 3000
     # targetPort: 4181 To be used with a proxy extraContainer
//}

//terminal{
$ helm install --name grafana --namespace monitoring -f grafana-values.yml stable/grafana
//}

//terminal{
$ minikube service -n monitoring grafana --url
http://192.168.99.100:30453
//}

//terminal{
$ kubectl get secret --namespace monitoring grafana -o jsonpath="{.data.admin-password}" | base64 --decode ; echo
xxxxxxxxxxxxxxxxxxxxxxxxxxxx
//}


=== etcdのモニタリング

Prometheusにはサービスディスカバリを利用して、自動的にモニタリングの対象をみつける機能があります。
詳しくは以下参照。
https://prometheus.io/docs/prometheus/latest/configuration/configuration/#kubernetes_sd_config

この機能を利用してetcdのメトリクスも収集してもらいましょう。
このとき、etcdのPodにアノテーションを付与する必要があります。

また、

//list[etcd-cluster][etcd-cluster.yml]{
#@mapfile(../code/chapter5/prometheus/etcd-cluster.yml)
apiVersion: "etcd.database.coreos.com/v1beta2"
kind: "EtcdCluster"
metadata:
  name: "example-etcd-cluster"
spec:
  size: 3
  version: "3.3.12"
  pod:
    annotations:
      prometheus.io/scrape: 'true'
      prometheus.io/port: '2379'
      prometheus.io/metrics: /metrics
    etcdEnv:
      - name: ETCD_METRICS
        value: "extensive"
#@end
//}

//terminal{
$ kubectl apply -f etcd-cluster.yml
//}

どのようなメトリクスが取れるのか実行してみましょう。

//terminal{
$ ETCD_ENDPOINT=$(minikube service example-etcd-cluster-client-service --url)
$ curl $ETCD_ENDPOINT/metrics
//}

メトリクス情報が正しく表示されたでしょうか？非常にたくさんの項目が存在することがわかると思います。
etcdで取得できるメトリクスは頻繁に仕様が変化している@<fn>{metrics3.4}ため、自身の環境でどのようなメトリクスが収集されているのかを把握しておくことが大切です。

//footnote[metrics3.4][etcd v3.4でも多くのメトリクスの追加が予定されています。]


@<code>{up\{app="etcd"\}}

//terminal{
up{app="etcd",etcd_cluster="example-etcd-cluster",etcd_node="example-etcd-cluster-lsrhl4k5lk",instance="172.17.0.13:2379",job="kubernetes-pods",kubernetes_namespace="default",kubernetes_pod_name="example-etcd-cluster-lsrhl4k5lk"}      1
up{app="etcd",etcd_cluster="example-etcd-cluster",etcd_node="example-etcd-cluster-pc8q9hrkv6",instance="172.17.0.14:2379",job="kubernetes-pods",kubernetes_namespace="default",kubernetes_pod_name="example-etcd-cluster-pc8q9hrkv6"}      1
up{app="etcd",etcd_cluster="example-etcd-cluster",etcd_node="example-etcd-cluster-ql7lhq9pcf",instance="172.17.0.15:2379",job="kubernetes-pods",kubernetes_namespace="default",kubernetes_pod_name="example-etcd-cluster-ql7lhq9pcf"}      1
//}

//terminal{
etcd_server_has_leader{app="etcd",etcd_cluster="example-etcd-cluster",etcd_node="example-etcd-cluster-cv9q5clmww",instance="172.17.0.14:2379",job="kubernetes-pods",kubernetes_namespace="default",kubernetes_pod_name="example-etcd-cluster-cv9q5clmww"}
etcd_server_has_leader{app="etcd",etcd_cluster="example-etcd-cluster",etcd_node="example-etcd-cluster-ggt54cqqlc",instance="172.17.0.13:2379",job="kubernetes-pods",kubernetes_namespace="default",kubernetes_pod_name="example-etcd-cluster-ggt54cqqlc"}
etcd_server_has_leader{app="etcd",etcd_cluster="example-etcd-cluster",etcd_node="example-etcd-cluster-sbc488shd4",instance="172.17.0.15:2379",job="kubernetes-pods",kubernetes_namespace="default",kubernetes_pod_name="example-etcd-cluster-sbc488shd4"}
//}

 * alertmanagerとprometheus-serverのサービスをNodePortに変更
 * alertのrulesを追加

//list[][prometheus-values.yml]{
#@maprange(../code/chapter5/prometheus/prometheus-values.yml,noleader)
serverFiles:
  alerts:
    groups:
      - name: etcd
        rules:
          - alert: LackMember
            expr: absent(up{app="etcd", etcd_cluster="example-etcd-cluster"}) OR count(up{app="etcd", etcd_cluster="example-etcd-cluster"}==1} < 2
            for: 1m
            labels:
              severity: critical
            annotations:
              summary: "etcd cluster member lost"
              description: "{{ $labels.etcd_cluster }}, member: {{ $labels.etcd_node }}"
          - alert: NoLeader
            expr: etcd_server_has_leader{app="etcd"} != 0
            for: 1m
            labels:
              severity: critical
            annotations:
              summary: "etcd cluster lost leader"
              description: "{{ $labels.etcd_cluster }} has no leader"
#@end
//}

 * helmでprometheusの設定を更新

//terminal{
helm upgrade --namespace monitoring -f prometheus-values.yml prometheus stable/prometheus
//}

 * prometheus-serverをブラウザで開いてみる。alertsタブを開いて設定したアラートがあれば成功。
 * アラートの発生条件を達成するとActiveになる。

//terminal{
$ minikube service -n monitoring prometheus-server --url
http://192.168.99.100:31149
//}

 * alertmanagerをブラウザで開く。
 * アラートが発生していれば、アラートが表示される。ここまで確認しなくてもいいか？

//terminal{
$ minikube service -n monitoring prometheus-alertmanager --url                                           │
http://192.168.99.100:32624 
//}

== gofail

@<hd>{取りこぼしを防ぐ}で紹介したプログラムでは、取りこぼしを防ぐことはできるが、重複処理が発生する可能性はあると説明しました。
ではどのような状況で重複処理が発生するのでしょうか？
もう一度コードを見てみましょう。

//list[?][]{
#@maprange(../code/chapter2/watch_file/watch_file.go,watch_file)
    rev := nextRev()
    fmt.Printf("loaded revision: %d\n", rev)
    ch := client.Watch(context.TODO(), "/chapter2/watch_file", clientv3.WithRev(rev))
    for resp := range ch {
        if resp.Err() != nil {
            log.Fatal(resp.Err())
        }
        for _, ev := range resp.Events {
            fmt.Printf("[%d] %s %q : %q\n", ev.Kv.ModRevision, ev.Type, ev.Kv.Key, ev.Kv.Value)
            doSomething(ev)
            err := saveRev(ev.Kv.ModRevision)
            if err != nil {
                log.Fatal(err)
            }
        }
    }
#@end
//}

このコードでは@<code>{doSomething()}関数で何か重要な処理をおこない、
@<code>{saveRev()}で処理が完了した値のリビジョンをファイルに書き出しています。
この@<code>{doSomething()}と@<code>{saveRev()}の間にプログラムがクラッシュしてしまうと、
処理が完了した値のリビジョンとファイルに保存したリビジョンに不整合が発生してしまい、
再度このプログラムを実行すると、すでに処理が完了したはずのデータが再度実行されてしまいます。

そんなタイミングでクラッシュするのか？と思われるかもしれませんが、人間がプロセスをkillしたり、OOMによって停止させられたり、
停電が発生することも考えられます。
今回の例ではファイルの書き出しをおこなっているので、ストレージが故障したり空き容量がない場合も考えられます。
信頼性の高いシステムを構築する場合には、様々な異常事態を想定してプログラムを実装する必要があります。

ところで、そのような想定をしたプログラムを実装したとしても、どのようにテストすればいいのでしょうか？
[gofail](https://github.com/coreos/gofail)は、etcdの開発チームがつくったFailure Injectionのためのツールです。
Go言語で書かれたプログラム中に故意にエラーを発生させるポイント(failpoint)を埋め込み、任意のタイミングでプログラムの挙動を変えることができます。

=== gofailを使ってみよう

gofail をインストールします。

//terminal{
$ go get -u github.com/etcd-io/gofail/...
//}

プログラム中にfailpointを埋め込むためには、プログラム中に@<code>{gofail}というキーワードから始まるコメントを記述します。
なお、ブロックコメント内に記述してもfailpointとして認識されないため、必ず行コメントで記述する必要があります。

また、mainパッケージ内にはfailpointを記述することはできません。


//list[?][]{
#@maprange(../code/chapter5/failpoint/failpoint.go,watch)
    rev := nextRev()
    fmt.Printf("loaded revision: %d\n", rev)
    ch := client.Watch(context.TODO(), "/chapter2/watch_file", clientv3.WithRev(rev))
    for resp := range ch {
        if resp.Err() != nil {
            log.Fatal(resp.Err())
        }
        for _, ev := range resp.Events {
            fmt.Printf("[%d] %s %q : %q\n", ev.Kv.ModRevision, ev.Type, ev.Kv.Key, ev.Kv.Value)
            doSomething(ev)
            // gofail: var ExampleOneLine struct{}
            err := saveRev(ev.Kv.ModRevision)
            if err != nil {
                log.Fatal(err)
            }
        }
    }
#@end
//}

failpointを埋め込んでビルドするには、まず下記のコマンドを実行します。

//terminal{
$ gofail enable
//}

するとgofailから始まるコメントを記述した箇所が書き換えられ、`*.fail.go` というファイルが生成されます。
あとは生成されたコードを含めて通常通りにビルドします。


埋め込まれたfailpointは初期状態ではoffになっているので、実際にfailさせるためにはfailpointの有効化をおこなう必要があります。
failpointを有効化させる手段としては、プログラムの起動時に環境変数で指定するかHTTP APIで有効化するか2通りの方法がありますが、ここではHTTP APIを利用した方法を紹介します。

まずgofailのエンドポイントを環境変数で指定してプログラムを起動します。

//terminal{
GOFAIL_HTTP="127.0.0.1:1234" ./cmd
//}

下記のAPIを叩くと、failpointの一覧を取得することができます。

//terminal{
$ curl http://127.0.0.1:1234/
my/package/path/SomeFuncString=
my/package/path/ExampleOneLine=
my/package/path/ExampleLabels=
//}

failpointを有効化するためには下記のAPIを実行します。
ここでは、`SomeFuncString`が `hello`という文字列を返すように設定しています。

//terminal{
$ curl http://127.0.0.1:1234/my/package/path/SomeFuncString -XPUT -d'return("hello")'
//}

再度下記のAPIを実行すると、failpointが設定されていることが確認できます。

//terminal{
$ curl http://127.0.0.1:1234/
my/package/path/SomeFuncString=return("hello")
my/package/path/ExampleOneLine=
my/package/path/ExampleLabels=
//}

この状態で`someFunc()`関数が実行されると、`hello`という文字列が返ってくるようになります。

failpointを無効化するには下記のAPIを実行します。

//terminal{
$ curl http://127.0.0.1:1234/my/package/path/SomeFuncString -XDELETE
//}


書き換えられたコードをもとに戻すには、以下のコマンドを実行します。
必ずもとに戻しておきましょう。

//terminal{
$ gofail disable
//}

=== failpointで実行可能なアクション

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

//list[?][]{
func someFunc() string {
    // gofail: var SomeFuncString string
    // return SomeFuncString
    return "default"
}
//}

なお、failpointとなる変数の型は `bool`, `int`, `string`, `struct{}` のみになります。

fail発生後に任意の処理を実行する必要がない場合(panicを発生させる場合など)は、以下のように `struct {}` 型の変数宣言のみを記述します。

//list[?][]{
func ExampleOneLineFunc() string {
    // gofail: var ExampleOneLine struct{}
    return "abc"
}
//}

failpointを有効化した時にgotoやラベル付きcontinue文を実行したい場合は、下記のように gofail の後ろにラベル名を記述することもできます。

//list[?][]{
func ExampleRetry() {
    // gofail: RETRY:
    for i := 0; ; i++ {
        fmt.Println(i)
        time.Sleep(time.Second)
        // gofail: var RetryLabel struct{}
        // goto RETRY
    }
}
//}

また、failを発生させる回数や確率を指定することもできます。

下記の例では、failpointが呼び出された最初の3回は`hello`という文字列を返し、その後は通常の状態に戻ります。

//terminal{
$ curl http://127.0.0.1:1234/my/package/path/SomeFuncString -XPUT -d'3*return("hello")'
//}

下記の例では、50%の確率で`hello`を返すようになります。

//terminal{
$ curl http://127.0.0.1:1234/my/package/path/SomeFuncString -XPUT -d'50.0%return("hello")'
//}
