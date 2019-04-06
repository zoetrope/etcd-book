package main

import (
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
	session, err := concurrency.NewSession(client, concurrency.WithTTL(10))
	if err != nil {
		log.Fatal(err)
	}
	select {
	case <-session.Done():
		log.Fatal("session has been orphaned")
	}
	//#@@range_end(lock)

}
