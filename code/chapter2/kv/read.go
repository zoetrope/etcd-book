package main

import (
	"context"
	"fmt"

	"github.com/coreos/etcd/clientv3"
)

func read(client *clientv3.Client) error {
	key := "test"
	ctx := context.Background()
	resp, err := client.Get(ctx, key)
	if err != nil {
		return err
	}
	if resp.Count == 0 {
		return err
	}
	fmt.Println(resp.Kvs[0].Value)
	return nil
}
