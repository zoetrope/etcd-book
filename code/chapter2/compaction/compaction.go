package main

import (
	"context"
	"fmt"
	"log"
	"time"

	"github.com/coreos/etcd/clientv3"
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

	client.Delete(context.TODO(), "/chapter2/compaction")
	//#@@range_begin(history)
	client.Put(context.TODO(), "/chapter2/compaction", "hoge")
	client.Put(context.TODO(), "/chapter2/compaction", "fuga")
	client.Put(context.TODO(), "/chapter2/compaction", "fuga")

	resp, err := client.Get(context.TODO(), "/chapter2/compaction")
	if err != nil {
		log.Fatal(err)
	}

	for i := resp.Kvs[0].CreateRevision; i <= resp.Kvs[0].ModRevision; i++ {
		r, err := client.Get(context.TODO(), "/chapter2/compaction", clientv3.WithRev(i))
		if err != nil {
			log.Fatal(err)
		}
		fmt.Printf("rev: %d, value: %s\n", i, r.Kvs[0].Value)
	}
	//#@@range_end(history)

	//#@@range_begin(compaction)
	_, err = client.Compact(context.TODO(), resp.Kvs[0].ModRevision)
	if err != nil {
		log.Fatal(err)
	}
	fmt.Printf("compacted: %d\n", resp.Kvs[0].ModRevision)
	for i := resp.Kvs[0].CreateRevision; i <= resp.Kvs[0].ModRevision; i++ {
		r, err := client.Get(context.TODO(), "/chapter2/compaction", clientv3.WithRev(i))
		if err != nil {
			fmt.Printf("failed to get: %v\n", err)
			continue
		}
		fmt.Printf("rev: %d, value: %s\n", i, r.Kvs[0].Value)
	}
	//#@@range_end(compaction)
}
