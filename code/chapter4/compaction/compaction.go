package main

import (
	"context"
	"fmt"
	"log"
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

	//#@@range_begin(history)
	client.Delete(context.TODO(), "/chapter4/compaction")
	client.Put(context.TODO(), "/chapter4/compaction", "hoge")
	client.Put(context.TODO(), "/chapter4/compaction", "fuga")
	client.Put(context.TODO(), "/chapter4/compaction", "fuga")

	resp, err := client.Get(context.TODO(), "/chapter4/compaction")
	if err != nil {
		log.Fatal(err)
	}

	for i := resp.Kvs[0].CreateRevision; i <= resp.Kvs[0].ModRevision; i++ {
		r, err := client.Get(context.TODO(), "/chapter4/compaction", clientv3.WithRev(i))
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
	for i := resp.Kvs[0].CreateRevision; i <= resp.Kvs[0].ModRevision; i++ {
		r, err := client.Get(context.TODO(), "/chapter4/compaction", clientv3.WithRev(i))
		if err != nil {
			fmt.Println(err)
			continue
		}
		fmt.Printf("rev: %d, value: %s\n", i, r.Kvs[0].Value)
	}
	//#@@range_end(compaction)
}
