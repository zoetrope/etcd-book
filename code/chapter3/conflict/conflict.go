package main

import (
	"context"
	"fmt"
	"log"
	"strconv"
	"time"

	"github.com/coreos/etcd/clientv3"
)

//#@@range_begin(add)
func addValue(client *clientv3.Client, d int) {
	resp, _ := client.Get(context.TODO(), "/chapter3/conflict")
	value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
	value += d
	client.Put(context.TODO(), "/chapter3/conflict", strconv.Itoa(value))
}

//#@@range_end(add)

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

	//#@@range_begin(conflict)
	client.Put(context.TODO(), "/chapter3/conflict", "10")
	go addValue(client, 5)
	go addValue(client, -3)
	time.Sleep(1 * time.Second)
	resp, _ := client.Get(context.TODO(), "/chapter3/conflict")
	fmt.Println(string(resp.Kvs[0].Value))
	//#@@range_end(conflict)
}
