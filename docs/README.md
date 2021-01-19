# Preface

## What is etcd?

etcd is a distributed key-value store.
etcd was developed by CoreOS[^1] as a mechanism for multiple machines to agree on how to restart one by one while cooperating.
etcd has been adopted as a backend for various software due to its ease of use.
Its adoption as a backend for Kubernetes has brought it widespread attention and increased the community's size.
In December 2018, it was donated to the CNCF and is currently under active development in the open-source community.

[^1]: CoreOS was acquired by Red Hat in 2018.

etcd has the following features.

* **High Availability**:
  etcd allows you to build a high availability cluster.
  It means that if you build an etcd cluster with five nodes, the cluster can continue to run even if up to two nodes fail.

* **Strong Consistency**:
  etcd adopts a strong consistency model.
  It means that if data is successfully written to an etcd cluster, subsequent reads of the data will always read the values that were written.
  This feature may sound obvious, but some distributed key-value stores do not have strong consistency.
  For example, databases with eventually consistency will eventually reflect the values written, but it will take a little longer.
  Instead, they can be scaled out to achieve very high performance.
  On the other hand, etcd, which uses a strong consistency model, cannot be scaled out to improve performance.

etcd is implemented based on a distributed consensus algorithm called Raft.
Raft defines how to elect a leader from the cluster members, how to replicate logs, and how to do so safely in network failure or member failure.
The correct implementation of the algorithm guarantees that there will be only one leader in the cluster, that the order of the logs will not be changed, and that the data will not be inconsistent, even in a network failure.
Thus, high reliability can be achieved.
Raft focuses on being easy to understand, making it simpler than other distributed consensus algorithms such as Paxos.

In addition to reading and writing key values, etcd provides functions such as Watch (value change monitoring), Lease (key expiration), and Leader Election.
These functions are very useful when developing distributed systems.

On the other hand, etcd has its weaknesses.
It cannot handle large data (cluster data size is 2GB by default, maximum 8GB, maximum 1.5MB per record).
Also, increasing the number of members does not improve the performance, so it is not suitable for large data applications.

This book describes a programming technique to use the functions of etcd using Go language.

### Target Version

* Go: 1.15.7
* etcd: 3.4.14

### History

* 2021: English edition
* 2020/09/25: Support for etcd 3.4
* 2019/11/24: First release
