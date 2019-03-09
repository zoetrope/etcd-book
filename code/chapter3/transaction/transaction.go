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
	addValue := func(d int) {
	RETRY:
		resp, _ := client.Get(context.TODO(), "/chapter3/txn")
		rev := resp.Kvs[0].ModRevision
		value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
		value += d
		tresp, err := client.Txn(context.TODO()).
			If(clientv3.Compare(clientv3.ModRevision("/chapter3/txn"), "=", rev)).
			Then(clientv3.OpPut("/chapter3/txn", strconv.Itoa(value))).
			Else().
			Commit()
		if err != nil {
			return
		}
		if !tresp.Succeeded {
			goto RETRY
		}
	}
	client.Put(context.TODO(), "/chapter3/txn", "10")
	go addValue(5)
	go addValue(-3)
	time.Sleep(1 * time.Second)
	resp, _ := client.Get(context.TODO(), "/chapter3/txn")
	fmt.Println(string(resp.Kvs[0].Value))
	//#@@range_end(txn)
}
