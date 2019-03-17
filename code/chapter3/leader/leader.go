package main

import (
	"context"
	"flag"
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

	//#@@range_begin(leader)
	flag.Parse()
	if flag.NArg() != 1 {
		log.Fatal("usage: ./leader NAME")
	}
	name := flag.Arg(0)
	s, err := concurrency.NewSession(client, concurrency.WithLease(leaseID))
	if err != nil {
		log.Fatal(err)
	}
	e := concurrency.NewElection(s, "/chapter3/leader/")

	err = e.Campaign(context.TODO(), name)
	if err != nil {
		log.Fatal(err)
	}
	for i := 0; i < 5; i++ {
		fmt.Println(name + " is a leader.")
		time.Sleep(1 * time.Second)
	}
	err = e.Resign(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
	//#@@range_end(leader)
}
