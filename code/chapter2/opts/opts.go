package main

import (
	"context"
	"fmt"
	"os"

	"github.com/coreos/etcd/clientv3"
)

func main() {
	cfg := clientv3.Config{
		Endpoints: []string{"http://localhost:2379"},
	}

	client, err := clientv3.New(cfg)
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}

	//#@@range_begin(write)
	_, err = client.Put(context.TODO(), "/mykey/key1", "foo")
	_, err = client.Put(context.TODO(), "/mykey/key2", "bar")
	_, err = client.Put(context.TODO(), "/mykey/key3", "hoge")
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	//#@@range_end(write)

	//#@@range_begin(read)
	resp, err := client.Get(context.TODO(), "/mykey/", clientv3.WithPrefix())
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	for _, kv := range resp.Kvs {
		fmt.Printf("%s: %s\n", kv.Key, kv.Value)
	}
	//#@@range_end(read)
}
