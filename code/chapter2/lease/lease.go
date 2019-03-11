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

	//#@@range_begin(lease)
	grantResp, err := client.Grant(context.TODO(), 5)
	if err != nil {
		log.Fatal(err)
	}
	_, err = client.Put(context.TODO(), "/chapter2/lease", "value",
		clientv3.WithLease(grantResp.ID))
	if err != nil {
		log.Fatal(err)
	}

	for {
		getResp, err := client.Get(context.TODO(), "/chapter2/lease")
		if err != nil {
			log.Fatal(err)
		}
		if getResp.Count == 0 {
			fmt.Println("'/chapter2/lease' disappeared")
			break
		}
		fmt.Printf("[%v] %s\n",
			time.Now().Format("15:04:05"), getResp.Kvs[0].Value)
		time.Sleep(1 * time.Second)
	}
	//#@@range_end(lease)
}
