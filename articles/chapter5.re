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

== etcdadm

== gofail
