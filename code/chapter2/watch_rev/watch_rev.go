package main

import (
	"context"
	"fmt"
	"log"
	"strconv"
	"time"

	"github.com/coreos/etcd/clientv3"
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
	go func() {
		for i := 0; i < 10; i++ {
			client.Put(context.TODO(), "/chapter2/watch_rev", strconv.Itoa(i))
			time.Sleep(100 * time.Millisecond)
		}
	}()
	time.Sleep(300 * time.Millisecond)

	//#@@range_begin(watch_rev)
	resp, err := client.Get(context.TODO(), "/chapter2/watch_rev")
	if err != nil || resp.Count == 0 {
		log.Fatal(err)
	}
	fmt.Printf("processed: %s\n", resp.Kvs[0].Value)
	rev := resp.Kvs[0].ModRevision
	time.Sleep(300 * time.Millisecond)
	ch := client.Watch(context.TODO(), "/chapter2/watch_rev", clientv3.WithRev(rev+1))
	for resp := range ch {
		if resp.Err() != nil {
			log.Fatal(resp.Err())
		}
		for _, ev := range resp.Events {
			fmt.Printf("%s %q : %q\n", ev.Type, ev.Kv.Key, ev.Kv.Value)
		}
	}
	//#@@range_end(watch_rev)
}
