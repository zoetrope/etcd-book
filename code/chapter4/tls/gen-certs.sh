#!/bin/bash -e

mkdir -p certs

# generate CA
cfssl gencert -initca ca-csr.json | cfssljson -bare certs/ca -

# generate server certificate
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=server server.json | cfssljson -bare certs/server

cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=peer etcd1.json | cfssljson -bare certs/etcd1
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=peer etcd2.json | cfssljson -bare certs/etcd2
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=peer etcd3.json | cfssljson -bare certs/etcd3

cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=client client.json | cfssljson -bare certs/client
