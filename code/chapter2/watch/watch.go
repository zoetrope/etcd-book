package main

import (
	"context"
	"fmt"
	"log"
	"time"

	"github.com/coreos/etcd/clientv3"
	"github.com/coreos/etcd/mvcc/mvccpb"
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

	//#@@range_begin(watch)
	rch := client.Watch(context.TODO(), "/chapter2/watch", clientv3.WithPrefix())
	for wresp := range rch {
		wresp.Err()
		wresp.IsProgressNotify()
		for _, ev := range wresp.Events {
			if ev.Type == mvccpb.PUT {

			}
			if ev.Type == mvccpb.DELETE {

			}
			fmt.Printf("%s %q : %q\n", ev.Type, ev.Kv.Key, ev.Kv.Value)
		}
	}
	//#@@range_end(watch)
}
