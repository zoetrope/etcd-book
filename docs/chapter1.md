# etcdに触れてみよう

## etcdを起動してみよう

それでは早速etcdを起動してみましょう。
今回はDockerを利用してetcdを起動するので、まずはDockerがインストールされていることを確認しましょう。

```
$ docker -v
Docker version 19.03.12, build 48a66213fe
```

Dockerがインストールされていない場合は下記のページを参考にインストールをおこなってください。

* https://docs.docker.com/get-docker/

次にetcdを起動します。

```
$ docker run --name etcd \
    -p 2379:2379 \
    --volume=etcd-data:/etcd-data \
    --name etcd gcr.io/etcd-development/etcd:v3.4.13 \
    /usr/local/bin/etcd \
      --name=etcd-1 \
      --data-dir=/etcd-data \
      --advertise-client-urls http://0.0.0.0:2379 \
      --listen-client-urls http://0.0.0.0:2379
```

Dockerに以下のオプションを指定します。

* `-p 2379:2379`
    コンテナ外からetcdのAPIを呼び出せるように2379番ポートをホストにバインドしています。
* `--volume=etcd-data:/etcd-data`
    etcd-dataというボリュームを作成し、コンテナの`/etcd-data`ディレクトリにバインドしています。これによりコンテナを終了してもetcdのデータは消えません。

etcdに以下の起動オプションを指定します。

* `--name`
    etcdのメンバーの名前を指定します。
* `--data-dir`
    etcdがデータを保存するディレクトリを指定します。
* `--listen-client-urls`
    etcdがクライアントからのリクエストを受け付けるURLを指定します。
* `--advertise-client-urls`
    クライアント用のURLをクラスタの他のメンバーに伝えるために指定します。
    現在はクラスタを組んでいませんが、`--listen-client-urls`を指定した場合は必ずこのオプションも指定する必要があります。

以下のようなログが出力されればetcdの起動は成功です。

```
{"level":"info","ts":"2020-09-25T03:23:44.379Z","caller":"embed/etcd.go:117","msg":"configuring peer listeners","listen-peer-urls":["http://localhost:2380"]}
{"level":"info","ts":"2020-09-25T03:23:44.380Z","caller":"embed/etcd.go:127","msg":"configuring client listeners","listen-client-urls":["http://0.0.0.0:2379"]}
{"level":"info","ts":"2020-09-25T03:23:44.380Z","caller":"embed/etcd.go:302","msg":"starting an etcd server","etcd-version":"3.4.13","git-sha":"ae9734ed2","go-version":"go1.12.17","go-os":"linux","go-arch":"amd64","max-cpu-set":16,"max-cpu-available":16,"member-initialized":false,"name":"etcd-1","data-dir":"/etcd-data","wal-dir":"","wal-dir-dedicated":"","member-dir":"/etcd-data/member","force-new-cluster":false,"heartbeat-interval":"100ms","election-timeout":"1s","initial-election-tick-advance":true,"snapshot-count":100000,"snapshot-catchup-entries":5000,"initial-advertise-peer-urls":["http://localhost:2380"],"listen-peer-urls":["http://localhost:2380"],"advertise-client-urls":["http://0.0.0.0:2379"],"listen-client-urls":["http://0.0.0.0:2379"],"listen-metrics-urls":[],"cors":["*"],"host-whitelist":["*"],"initial-cluster":"etcd-1=http://localhost:2380","initial-cluster-state":"new","initial-cluster-token":"etcd-cluster","quota-size-bytes":2147483648,"pre-vote":false,"initial-corrupt-check":false,"corrupt-check-time-interval":"0s","auto-compaction-mode":"periodic","auto-compaction-retention":"0s","auto-compaction-interval":"0s","discovery-url":"","discovery-proxy":""}
2020-09-25 03:23:44.380211 W | pkg/fileutil: check file permission: directory "/etcd-data" exist, but the permission is "drwxr-xr-x". The recommended permission is "-rwx------" to prevent possible unprivileged access to the data.
{"level":"info","ts":"2020-09-25T03:23:44.382Z","caller":"etcdserver/backend.go:80","msg":"opened backend db","path":"/etcd-data/member/snap/db","took":"1.9665ms"}
{"level":"info","ts":"2020-09-25T03:23:44.386Z","caller":"etcdserver/raft.go:486","msg":"starting local member","local-member-id":"8e9e05c52164694d","cluster-id":"cdf818194e3a8c32"}
{"level":"info","ts":"2020-09-25T03:23:44.386Z","caller":"raft/raft.go:1530","msg":"8e9e05c52164694d switched to configuration voters=()"}
{"level":"info","ts":"2020-09-25T03:23:44.386Z","caller":"raft/raft.go:700","msg":"8e9e05c52164694d became follower at term 0"}
{"level":"info","ts":"2020-09-25T03:23:44.387Z","caller":"raft/raft.go:383","msg":"newRaft 8e9e05c52164694d [peers: [], term: 0, commit: 0, applied: 0, lastindex: 0, lastterm: 0]"}
{"level":"info","ts":"2020-09-25T03:23:44.387Z","caller":"raft/raft.go:700","msg":"8e9e05c52164694d became follower at term 1"}
{"level":"info","ts":"2020-09-25T03:23:44.387Z","caller":"raft/raft.go:1530","msg":"8e9e05c52164694d switched to configuration voters=(10276657743932975437)"}
{"level":"warn","ts":"2020-09-25T03:23:44.389Z","caller":"auth/store.go:1366","msg":"simple token is not cryptographically signed"}
{"level":"info","ts":"2020-09-25T03:23:44.392Z","caller":"etcdserver/quota.go:98","msg":"enabled backend quota with default value","quota-name":"v3-applier","quota-size-bytes":2147483648,"quota-size":"2.1 GB"}
{"level":"info","ts":"2020-09-25T03:23:44.393Z","caller":"etcdserver/server.go:803","msg":"starting etcd server","local-member-id":"8e9e05c52164694d","local-server-version":"3.4.13","cluster-version":"to_be_decided"}
{"level":"info","ts":"2020-09-25T03:23:44.393Z","caller":"etcdserver/server.go:669","msg":"started as single-node; fast-forwarding election ticks","local-member-id":"8e9e05c52164694d","forward-ticks":9,"forward-duration":"900ms","election-ticks":10,"election-timeout":"1s"}
{"level":"info","ts":"2020-09-25T03:23:44.394Z","caller":"raft/raft.go:1530","msg":"8e9e05c52164694d switched to configuration voters=(10276657743932975437)"}
{"level":"info","ts":"2020-09-25T03:23:44.394Z","caller":"membership/cluster.go:392","msg":"added member","cluster-id":"cdf818194e3a8c32","local-member-id":"8e9e05c52164694d","added-peer-id":"8e9e05c52164694d","added-peer-peer-urls":["http://localhost:2380"]}
{"level":"info","ts":"2020-09-25T03:23:44.395Z","caller":"embed/etcd.go:244","msg":"now serving peer/client/metrics","local-member-id":"8e9e05c52164694d","initial-advertise-peer-urls":["http://localhost:2380"],"listen-peer-urls":["http://localhost:2380"],"advertise-client-urls":["http://0.0.0.0:2379"],"listen-client-urls":["http://0.0.0.0:2379"],"listen-metrics-urls":[]}
{"level":"info","ts":"2020-09-25T03:23:44.395Z","caller":"embed/etcd.go:579","msg":"serving peer traffic","address":"127.0.0.1:2380"}
{"level":"info","ts":"2020-09-25T03:23:45.287Z","caller":"raft/raft.go:923","msg":"8e9e05c52164694d is starting a new election at term 1"}
{"level":"info","ts":"2020-09-25T03:23:45.287Z","caller":"raft/raft.go:713","msg":"8e9e05c52164694d became candidate at term 2"}
{"level":"info","ts":"2020-09-25T03:23:45.287Z","caller":"raft/raft.go:824","msg":"8e9e05c52164694d received MsgVoteResp from 8e9e05c52164694d at term 2"}
{"level":"info","ts":"2020-09-25T03:23:45.287Z","caller":"raft/raft.go:765","msg":"8e9e05c52164694d became leader at term 2"}
{"level":"info","ts":"2020-09-25T03:23:45.287Z","caller":"raft/node.go:325","msg":"raft.node: 8e9e05c52164694d elected leader 8e9e05c52164694d at term 2"}
{"level":"info","ts":"2020-09-25T03:23:45.287Z","caller":"etcdserver/server.go:2037","msg":"published local member to cluster through raft","local-member-id":"8e9e05c52164694d","local-member-attributes":"{Name:etcd-1 ClientURLs:[http://0.0.0.0:2379]}","request-path":"/0/members/8e9e05c52164694d/attributes","cluster-id":"cdf818194e3a8c32","publish-timeout":"7s"}
{"level":"info","ts":"2020-09-25T03:23:45.287Z","caller":"etcdserver/server.go:2528","msg":"setting up initial cluster version","cluster-version":"3.4"}
{"level":"info","ts":"2020-09-25T03:23:45.288Z","caller":"embed/serve.go:139","msg":"serving client traffic insecurely; this is strongly discouraged!","address":"[::]:2379"}
{"level":"info","ts":"2020-09-25T03:23:45.288Z","caller":"membership/cluster.go:558","msg":"set initial cluster version","cluster-id":"cdf818194e3a8c32","local-member-id":"8e9e05c52164694d","cluster-version":"3.4"}
{"level":"info","ts":"2020-09-25T03:23:45.288Z","caller":"api/capability.go:76","msg":"enabled capabilities for version","cluster-version":"3.4"}
{"level":"info","ts":"2020-09-25T03:23:45.288Z","caller":"etcdserver/server.go:2560","msg":"cluster version is updated","cluster-version":"3.4"}
```

次にetcdctlを使います。etcdctlはetcdとやり取りするためのコマンドラインツールです。
下記のetcdのリリースページから、利用しているOS用のバイナリをダウンロードしてください。

* https://github.com/etcd-io/etcd/releases

etcdctlコマンドを実行してみましょう。

```
$ etcdctl
NAME:
        etcdctl - A simple command line client for etcd3.

USAGE:
        etcdctl [flags]

VERSION:
        3.4.13

API VERSION:
        3.4


COMMANDS:
        以下省略
```

なお、etcdはAPIのバージョンとしてv2とv3をサポートしており、etcdctlはv3.4からデフォルトでAPI v3を利用するようになりました。
v3.3以前のバージョンを利用している場合は、API v3を利用するために環境変数`ETCDCTL_API=3`を指定する必要があります。

## etcdにデータを読み書きしてみよう

etcdctlを利用してetcdにデータを書き込んでみます。
ここでは`/chapter2/hello`というキーに`Hello, World!`といバリューを書き込んでいます。
書き込みに成功するとOKと表示されます。

```
$ etcdctl put /chapter2/hello 'Hello, World!'
OK
```

次にキーを指定して値を読み込んでみましょう。
`/chapter2/hello`というキーを指定してgetコマンドを実行すると、先ほど書き込んだバリューが表示されます。

```
$ etcdctl get /chapter2/hello
/chapter2/hello
Hello, World!
```

### キー

先程の例でetcdにデータを書き込むときに`/chapter2/hello`というキーを利用しました。
このキーの名前はどのように決めればいいのでしょうか？

etcd v3ではキーは内部的にはバイト配列として扱われており、どのような文字列でも利用することができます。
日本語を利用することも可能です。

:::message
etcd v2とv3ではキーの管理方法が大きく変わりました。
etcd v2ではキーをファイルシステムのように/で区切って階層的に管理していました。
etcd v3からはキーを単一のバイト配列としてフラットなキースペースで管理するようになりました。
:::

etcdには、MySQLやPostgreSQLにおけるデータベースやテーブルのような概念はありません。
そのため複数のアプリケーションで1つのetcdクラスタを共有する場合は、アプリケーションごとにキーのプレフィックスを決めておくことが一般的です。
例えばアプリケーション1は`/app1/`から始まるキー、アプリケーション2は`/app2/`から始まるキーを使うというような決まりにします。
etcdではキーのプレフィックスに対してアクセス権を設定することができるので、`/app1/`から始まるキーはアプリケーション1の実行ユーザーのみが読み書き可能な設定にしておきます。

具体的にKubernetesの例を見てみましょう。
Kubernetesが動いている環境で、etcdctlを利用してキーの一覧を取得してみます。

```
$ etcdctl get / --prefix=true --keys-only
/registry/pods/default/mypod
/registry/services/kube-system/nginx
  以下省略
```

このようにKubernetesは`/registry/`から始まるキーを利用しています。
その後ろにリソースのタイプ(podやservice)、ネームスペース(defaultやkube-system)、リソースの名前(mypodやnginx)を`/`でつないでキーにしています。

### バリュー

一方のバリューはどのような形式にするのがよいのでしょうか？
バリューも内部的にはバイト配列として扱われています。
1つのバリューのサイズは最大1.5MBまで格納することができます。

バリューは単純な文字列として格納してもよいですし、構造的なデータを扱いたいのであれば
JSONやProtocol Bufferなどでエンコードした形式で格納してもよいでしょう。

具体的にKubernetesの例を見てみましょう。
先ほどと同じように、Kubernetesが動いている環境でetcdctlを利用してバリューを取得してみます。

```
$ etcdctl get /registry/pods/default/mypod
```

このコマンドを実行すると、バイナリ形式のデータが出力されます。

Kubernetesではv1.6以降はProtocol Buffer形式でデータを格納しているため、そのままでは読むことができません。
Auger[^3]などのツールを利用すればデコードして表示することも可能です。

[^3]: footnote[auger][https://github.com/jpbetz/auger

なお、etcdのデータファイルにはキーバリューの内容がそのまま平文で保存されています。
秘密情報などを管理したい場合はバリューを暗号化して格納する必要があります。
