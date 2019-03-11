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

	//#@@range_begin(write)
	_, err = client.Put(context.TODO(), "/chapter2/kv", "value")
	if err != nil {
		log.Fatal(err)
	}
	//#@@range_end(write)

	//#@@range_begin(read)
	resp, err := client.Get(context.TODO(), "/chapter2/kv")
	if err != nil {
		log.Fatal(err)
	}
	if resp.Count == 0 {
		log.Fatal(err)
	}
	fmt.Println(string(resp.Kvs[0].Value))
	//#@@range_end(read)
}
