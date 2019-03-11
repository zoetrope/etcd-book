package main

import (
	"context"
	"fmt"
	"log"
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
		log.Fatal(err)
	}
	defer client.Close()

	//#@@range_begin(tocttou)
	addValue := func(d int) {
		resp, _ := client.Get(context.TODO(), "/chapter3/tocttou")
		value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
		value += d
		client.Put(context.TODO(), "/chapter3/tocttou", strconv.Itoa(value))
	}
	client.Put(context.TODO(), "/chapter3/tocttou", "10")
	go addValue(5)
	go addValue(-3)
	time.Sleep(1 * time.Second)
	resp, _ := client.Get(context.TODO(), "/chapter3/tocttou")
	fmt.Println(string(resp.Kvs[0].Value))
	//#@@range_end(tocttou)
}
