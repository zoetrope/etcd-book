package main

import (
	"context"
	"flag"
	"log"
	"time"

	"github.com/coreos/etcd/clientv3"
	"github.com/coreos/etcd/clientv3/clientv3util"
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
	flag.Parse()
	if flag.NArg() != 1 {
		log.Fatal("usage: ./leader_txn NAME")
	}
	name := flag.Arg(0)
	s, err := concurrency.NewSession(client, concurrency.WithTTL(10))
	if err != nil {
		log.Fatal(err)
	}
	defer s.Close()
	e := concurrency.NewElection(s, "/chapter3/leader_txn/")

RETRY:
	select {
	case <-s.Done():
		log.Fatal("session has been orphaned")
	default:
	}
	err = e.Campaign(context.TODO(), name)
	if err != nil {
		log.Fatal(err)
	}
	leaderKey := e.Key()
	resp, err := s.Client().Txn(context.TODO()).
		If(clientv3util.KeyExists(leaderKey)).
		Then(clientv3.OpPut("/chapter3/leader_txn_value", "value")).
		Commit()
	if err != nil {
		log.Fatal(err)
	}
	if !resp.Succeeded {
		goto RETRY
	}

	err = e.Resign(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
	//#@@range_end(txn)
}
