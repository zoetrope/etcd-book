package main

import (
	"context"
	"fmt"
	"log"
	"os"
	"os/signal"
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

	//#@@range_begin(context)
	ch := make(chan os.Signal, 1)
	signal.Notify(ch, os.Interrupt)
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	go func() {
		<-ch
		cancel()
	}()

	for {
		_, err := client.Get(ctx, "/chapter2/context")
		if err != nil {
			fmt.Println(err)
			break
		}
		time.Sleep(1 * time.Second)
	}
	//#@@range_end(context)
}
