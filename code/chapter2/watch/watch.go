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

	//#@@range_begin(watch)
	ch := client.Watch(context.TODO(), "/chapter2/watch/", clientv3.WithPrefix())
	for resp := range ch {
		if resp.Err() != nil {
			log.Fatal(resp.Err())
		}
		for _, ev := range resp.Events {
			switch ev.Type {
			case clientv3.EventTypePut:
				switch {
				case ev.IsCreate():
					fmt.Printf("CREATE %q : %q\n", ev.Kv.Key, ev.Kv.Value)
				case ev.IsModify():
					fmt.Printf("MODIFY %q : %q\n", ev.Kv.Key, ev.Kv.Value)
				}
			case clientv3.EventTypeDelete:
				fmt.Printf("DELETE %q : %q\n", ev.Kv.Key, ev.Kv.Value)
			}
		}
	}
	//#@@range_end(watch)
}
