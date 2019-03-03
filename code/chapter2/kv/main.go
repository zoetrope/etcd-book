package main

import (
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
	err = write(client)
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	err = read(client)
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}
