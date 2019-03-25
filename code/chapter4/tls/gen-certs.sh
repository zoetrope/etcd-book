#!/bin/bash -e

VAULT="docker exec -e VAULT_TOKEN=myroot -e VAULT_ADDR=http://127.0.0.1:8200 dev-vault vault"

function create_ca(){
  common_name=$1
  $VAULT secrets enable -path ${common_name} -max-lease-ttl=876000h -default-lease-ttl=87600h pki
  s=$($VAULT write -format=json ${common_name}/root/generate/internal common_name=${common_name} ttl=876000h format=pem)
  echo ${s} | jq -r .data.certificate > /tmp/${common_name}
}

# docker run --cap-add=IPC_LOCK -e 'VAULT_DEV_ROOT_TOKEN_ID=myroot' -d --name=dev-vault vault:latest

# generate ca
#create_ca "ca-etcd-server"
#create_ca "ca-etcd-peer"
#create_ca "ca-etcd-client"
create_ca "hogehoge3"

# generate server certificate
$VAULT write -format=json ca-etcd-server/issue/system

# vault secrets enable -path "etcd-ca" -max-lease-ttl=876000h -default-lease-ttl=87600h pki
# s=$(vault write -format=json etcd-ca/root/generate/internal common_name=etcd-ca ttl=876000h format=pem)
# echo ${s} | jq -r .data.certificate > ./etcd-ca

# docker stop dev-vault
# docker rm dev-vault
