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

	resp, err := client.MemberList(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
	for _, m := range resp.Members {
		fmt.Printf("%s\n", m.String())
	}
}
