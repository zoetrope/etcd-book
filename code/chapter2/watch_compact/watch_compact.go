package main

import (
	"context"
	"fmt"
	"log"
	"strconv"
	"time"

	"github.com/coreos/etcd/clientv3"
	"github.com/coreos/etcd/etcdserver/api/v3rpc/rpctypes"
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
			client.Put(context.TODO(), "/chapter2/watch_compact", strconv.Itoa(i))
			time.Sleep(100 * time.Millisecond)
		}
	}()
	time.Sleep(300 * time.Millisecond)

	resp, err := client.Get(context.TODO(), "/chapter2/watch_compact")
	if err != nil || resp.Count == 0 {
		log.Fatal(err)
	}
	rev := resp.Kvs[0].ModRevision + 1

	time.Sleep(300 * time.Millisecond)

	resp, err = client.Get(context.TODO(), "/chapter2/watch_compact")
	if err != nil || resp.Count == 0 {
		log.Fatal(err)
	}
	_, err = client.Compact(context.TODO(), resp.Kvs[0].ModRevision)
	if err != nil {
		log.Fatal(err)
	}

	//#@@range_begin(watch_compact)
RETRY:
	fmt.Printf("watch from rev: %d\n", rev)
	ch := client.Watch(context.TODO(),
		"/chapter2/watch_compact", clientv3.WithRev(rev))
	for resp := range ch {
		if resp.Err() == rpctypes.ErrCompacted {
			rev = resp.CompactRevision
			goto RETRY
		} else if resp.Err() != nil {
			log.Fatal(resp.Err())
		}
		for _, ev := range resp.Events {
			fmt.Printf("%s %q : %q\n", ev.Type, ev.Kv.Key, ev.Kv.Value)
		}
	}
	//#@@range_end(watch_compact)
}
