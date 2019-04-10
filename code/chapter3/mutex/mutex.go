package main

import (
	"context"
	"fmt"
	"log"
	"time"

	"github.com/coreos/etcd/clientv3"
	"github.com/coreos/etcd/clientv3/concurrency"
)

func main() {
	client, err := clientv3.New(clientv3.Config{
		Endpoints:   []string{"http://localhost:2379"},
		DialTimeout: 3 * time.Second,
	})
	if err != nil {
		log.Fatal(err)
	}
	defer client.Close()

	//#@@range_begin(lock)
	session, err := concurrency.NewSession(client)
	if err != nil {
		log.Fatal(err)
	}
	mutex := concurrency.NewMutex(session, "/chapter3/mutex")
	err = mutex.Lock(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
	fmt.Println("acquired lock")
	time.Sleep(5 * time.Second)
	err = mutex.Unlock(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
	fmt.Println("released lock")
	//#@@range_end(lock)
}
