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

	resp, err := client.Status(context.Background(), "localhost:2379")
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	fmt.Printf("%#v\n", resp)
}
