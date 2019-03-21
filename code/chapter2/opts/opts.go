package main

import (
	"context"
	"fmt"
	"log"
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
		log.Fatal(err)
	}
	defer client.Close()
	//#@@range_begin(read)
	client.Put(context.TODO(), "/chapter2/option/key3", "val2")
	client.Put(context.TODO(), "/chapter2/option/key1", "val3")
	client.Put(context.TODO(), "/chapter2/option/key2", "val1")
	resp, err := client.Get(context.TODO(), "/chapter2/option/",
		clientv3.WithPrefix(),
		clientv3.WithSort(clientv3.SortByValue, clientv3.SortAscend),
		clientv3.WithKeysOnly(),
	)
	if err != nil {
		log.Fatal(err)
	}
	for _, kv := range resp.Kvs {
		fmt.Printf("%s: %s\n", kv.Key, kv.Value)
	}
	//#@@range_end(read)
}
