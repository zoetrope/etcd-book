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

	//#@@range_begin(txn)
	addValue := func(d int) func(stm concurrency.STM) error {
		return func(stm concurrency.STM) error {
			v := stm.Get("/chapter3/stm")
			value, err := strconv.Atoi(v)
			if err != nil {
				return err
			}
			value += d
			stm.Put("/chapter3/stm", strconv.Itoa(value))
			return nil
		}
	}
	client.Put(context.TODO(), "/chapter3/stm", "10")
	go func(){
		if _, err := concurrency.NewSTM(client, addValue(5)); err != nil {
			log.Fatal(err)
		}
	}()
	go func(){
		if _, err := concurrency.NewSTM(client, addValue(-3)); err != nil {
			log.Fatal(err)
		}
	}()
	
	time.Sleep(1 * time.Second)
	resp, _ := client.Get(context.TODO(), "/chapter3/stm")
	fmt.Println(string(resp.Kvs[0].Value))
	//#@@range_end(txn)
}
