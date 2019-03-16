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
		DialTimeout: 10 * time.Second,
	}

	client, err := clientv3.New(cfg)
	if err != nil {
		log.Fatal(err)
	}
	defer client.Close()

	//#@@range_begin(timeout)
	ctx, cancel := context.WithTimeout(context.Background(), 1*time.Second)
	defer cancel()
	resp, err := client.Get(ctx, "/chapter2/timeout")
	if err != nil {
		log.Fatal(err)
	}
	fmt.Println(resp)
	//#@@range_end(timeout)
}
