#!/bin/bash -e

mkdir -p certs

# generate CA
#@range_begin(ca)
cfssl gencert -initca ca-csr.json | cfssljson -bare certs/ca
#@range_end(ca)

# generate server certificate
#@range_begin(server)
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=server server.json | cfssljson -bare certs/server
#@range_end(server)

# generate peer certificate
#@range_begin(peer)
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=peer etcd1.json | cfssljson -bare certs/etcd1
#@range_end(peer)
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=peer etcd2.json | cfssljson -bare certs/etcd2
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=peer etcd3.json | cfssljson -bare certs/etcd3

# generate client certificate
#@range_begin(client)
cfssl gencert -ca=certs/ca.pem -ca-key=certs/ca-key.pem -config=ca-config.json -profile=client client.json | cfssljson -bare certs/client
#@range_end(client)
