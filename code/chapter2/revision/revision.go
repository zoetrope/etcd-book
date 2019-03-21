package main

import (
	"context"
	"fmt"
	"log"
	"time"

	"github.com/coreos/etcd/clientv3"
)

//#@@range_begin(print)
func printResponse(resp *clientv3.GetResponse) {
	fmt.Println("header: " + resp.Header.String())
	for i, kv := range resp.Kvs {
		fmt.Printf("kv[%d]: %s\n", i, kv.String())
	}
	fmt.Println()
}

//#@@range_end(print)

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

	client.Delete(context.TODO(), "/chapter2/rev/", clientv3.WithPrefix())
	//#@@range_begin(rev1)
	client.Put(context.TODO(), "/chapter2/rev/1", "123")
	resp, _ := client.Get(context.TODO(), "/chapter2/rev", clientv3.WithPrefix())
	printResponse(resp)
	//#@@range_end(rev1)

	//#@@range_begin(rev2)
	client.Put(context.TODO(), "/chapter2/rev/1", "456")
	resp, _ = client.Get(context.TODO(), "/chapter2/rev", clientv3.WithPrefix())
	printResponse(resp)
	//#@@range_end(rev2)

	//#@@range_begin(rev3)
	client.Put(context.TODO(), "/chapter2/rev/2", "999")
	resp, _ = client.Get(context.TODO(), "/chapter2/rev", clientv3.WithPrefix())
	printResponse(resp)
	//#@@range_end(rev3)

	//#@@range_begin(withrev)
	resp, _ = client.Get(context.TODO(), "/chapter2/rev",
		clientv3.WithPrefix(), clientv3.WithRev(resp.Kvs[0].CreateRevision))
	printResponse(resp)
	//#@@range_end(withrev)
}
