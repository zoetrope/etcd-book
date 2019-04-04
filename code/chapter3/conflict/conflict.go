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
func addValue(client *clientv3.Client, key string, d int) {
	resp, _ := client.Get(context.TODO(), key)
	value, _ := strconv.Atoi(string(resp.Kvs[0].Value))
	value += d
	client.Put(context.TODO(), key, strconv.Itoa(value))
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
	key := "/chapter3/conflict"
	client.Put(context.TODO(), key, "10")
	go addValue(client, key, 5)
	go addValue(client, key, -3)
	time.Sleep(1 * time.Second)
	resp, _ := client.Get(context.TODO(), key)
	fmt.Println(string(resp.Kvs[0].Value))
	//#@@range_end(conflict)
}
