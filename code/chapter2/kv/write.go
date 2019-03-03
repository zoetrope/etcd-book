package main

import (
	"context"

	"github.com/coreos/etcd/clientv3"
)

func write(client *clientv3.Client) error {
	key := "test"
	ctx := context.Background()
	_, err := client.Put(ctx, key, "value")
	if err != nil {
		return err
	}
	return nil
}
