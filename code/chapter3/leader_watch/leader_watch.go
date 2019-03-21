package main

import (
	"context"
	"errors"
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

	flag.Parse()
	if flag.NArg() != 1 {
		log.Fatal("usage: ./leader_watch NAME")
	}
	name := flag.Arg(0)
	s, err := concurrency.NewSession(client, concurrency.WithTTL(10))
	if err != nil {
		log.Fatal(err)
	}
	defer s.Close()
	e := concurrency.NewElection(s, "/chapter3/leader_watch")

	err = e.Campaign(context.TODO(), name)
	if err != nil {
		log.Fatal(err)
	}
	leaderKey := e.Key()
	fmt.Println("leader key: " + leaderKey)

	err = watchLeader(context.TODO(), s, leaderKey)
	if err != nil {
		log.Fatal(err)
	}
}

//#@@range_begin(watch)
func watchLeader(ctx context.Context, s *concurrency.Session, leaderKey string) error {
	ch := s.Client().Watch(ctx, leaderKey, clientv3.WithFilterPut())
	for {
		select {
		case <-s.Done():
			return errors.New("session is closed")
		case resp, ok := <-ch:
			if !ok {
				return errors.New("watch is closed")
			}
			if resp.Err() != nil {
				return resp.Err()
			}
			for _, ev := range resp.Events {
				if ev.Type == clientv3.EventTypeDelete {
					return errors.New("leader key is deleted")
				}
			}
		}
	}
}

//#@@range_end(watch)
