package main

import (
	"context"
	"fmt"
	"os"
	"strconv"
	"time"

	"github.com/coreos/etcd/clientv3"
)

func main() {
	cfg := clientv3.Config{
		Endpoints:   []string{"http://localhost:2379"},
		DialTimeout: 3 * time.Second,
	}

	client, err := clientv3.New(cfg)
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	defer client.Close()

	//#@@range_begin(txn)
	client.Put(context.TODO(), "/chapter2/txn", "10")
	go func() {
	RETRY:
		resp, _ := client.Get(context.TODO(), "/chapter2/txn")
		rev := resp.Kvs[0].ModRevision
		value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
		value += 5
		tresp, err := client.Txn(context.TODO()).
			If(clientv3.Compare(clientv3.ModRevision("/chapter2/txn"), "=", rev)).
			Then(clientv3.OpPut("/chapter2/txn", strconv.Itoa(value))).
			Commit()
		if err != nil {
			return
		}
		if !tresp.Succeeded {
			goto RETRY
		}
	}()
	go func() {
	RETRY:
		resp, _ := client.Get(context.TODO(), "/chapter2/txn")
		rev := resp.Kvs[0].ModRevision
		value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
		value -= 3
		tresp, err := client.Txn(context.TODO()).
			If(clientv3.Compare(clientv3.ModRevision("/chapter2/txn"), "=", rev)).
			Then(clientv3.OpPut("/chapter2/txn", strconv.Itoa(value))).
			Commit()
		if err != nil {
			return
		}
		if !tresp.Succeeded {
			goto RETRY
		}
	}()
	time.Sleep(1 * time.Second)
	resp, _ := client.Get(context.TODO(), "/chapter2/txn")
	fmt.Println(string(resp.Kvs[0].Value))
	//#@@range_end(txn)
}
