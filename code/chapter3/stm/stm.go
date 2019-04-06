package main

import (
	"context"
	"fmt"
	"log"
	"strconv"
	"time"

	"github.com/coreos/etcd/clientv3"
	"github.com/coreos/etcd/clientv3/concurrency"
)

//#@@range_begin(stm)
func addValue(client *clientv3.Client, key string, d int) {
	_, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
		v := stm.Get(key)
		value, _ := strconv.Atoi(v)
		value += d
		stm.Put(key, strconv.Itoa(value))
		return nil
	})
	if err != nil {
		log.Fatal(err)
	}
}

//#@@range_end(stm)

func main() {
	client, err := clientv3.New(clientv3.Config{
		Endpoints:   []string{"http://localhost:2379"},
		DialTimeout: 3 * time.Second,
	})
	if err != nil {
		log.Fatal(err)
	}
	defer client.Close()

	key := "/chapter3/stm"
	client.Put(context.TODO(), key, "10")
	go addValue(client, key, 5)
	go addValue(client, key, -3)

	time.Sleep(1 * time.Second)
	resp, _ := client.Get(context.TODO(), key)
	fmt.Println(string(resp.Kvs[0].Value))
}
