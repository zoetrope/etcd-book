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
	cfg := clientv3.Config{
		Endpoints:   []string{"http://localhost:2379"},
		DialTimeout: 3 * time.Second,
	}

	client, err := clientv3.New(cfg)
	if err != nil {
		log.Fatal(err)
	}
	defer client.Close()

	//#@@range_begin(lock)
	s, err := concurrency.NewSession(client)
	if err != nil {
		log.Fatal(err)
	}
	m := concurrency.NewMutex(s, "/chapter3/mutex")
	err = m.Lock(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
	fmt.Println("acquired lock")
	time.Sleep(5 * time.Second)
	err = m.Unlock(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
	fmt.Println("released lock")
	//#@@range_end(lock)

	//#@@range_begin(owner)
RETRY:
	select {
	case <-s.Done():
		log.Fatal("session has been orphaned")
	default:
	}
	err = m.Lock(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
	resp, err := client.Txn(context.TODO()).
		If(m.IsOwner()).
		Then(clientv3.OpPut("/chapter3/mutex/owner", "test")).
		Commit()
	if err != nil {
		log.Fatal(err)
	}
	if resp.Succeeded {
		fmt.Println("the lock was not acquired")
		m.Unlock(context.TODO())
		goto RETRY
	}
	// do something
	err = m.Unlock(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
	//#@@range_end(owner)
}
