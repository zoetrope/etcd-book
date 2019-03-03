package main

import (
	"context"
	"fmt"
	"os"

	"github.com/coreos/etcd/clientv3"
)

func main() {
	fmt.Println("hello")
	cfg := clientv3.Config{
		Endpoints: []string{"http://localhost:2379"},
	}

	client, err := clientv3.New(cfg)
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}

	key := "test"
	ctx := context.Background()
	resp, err := client.Get(ctx, key)
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	if resp.Count == 0 {
		fmt.Printf("key not found: %s", key)
		os.Exit(1)
	}
	fmt.Println(resp.Kvs[0].Value)
}
