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
	_, err = client.Put(context.Background(), "/mykey", "value")
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	//#@@range_end(write)

	//#@@range_begin(read)
	resp, err := client.Get(context.Background(), "/mykey")
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	if resp.Count == 0 {
		fmt.Println("not found")
		os.Exit(1)
	}
	fmt.Println(string(resp.Kvs[0].Value))
	//#@@range_end(read)
}
