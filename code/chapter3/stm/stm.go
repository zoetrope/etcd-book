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

	//#@@range_begin(stm)
	addValue := func(d int) {
		_, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
			v := stm.Get("/chapter3/stm")
			value, err := strconv.Atoi(v)
			if err != nil {
				return err
			}
			value += d
			stm.Put("/chapter3/stm", strconv.Itoa(value))
			return nil
		})
		if err != nil {
			log.Fatal(err)
		}
	}

	client.Put(context.TODO(), "/chapter3/stm", "10")
	go addValue(5)
	go addValue(-3)

	time.Sleep(1 * time.Second)
	resp, _ := client.Get(context.TODO(), "/chapter3/stm")
	fmt.Println(string(resp.Kvs[0].Value))
	//#@@range_end(stm)
}
