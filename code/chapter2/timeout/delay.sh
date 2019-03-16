#!/bin/sh

VETH=$(sudo ../../chapter6/veth.sh etcd)
echo $VETH

sudo tc qdisc del dev $VETH root
sudo tc qdisc add dev $VETH root netem delay 3s
