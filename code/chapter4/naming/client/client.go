//go:generate protoc -I ../pb --go_out=plugins=grpc:./ ../pb/helloworld.proto
package main

import (
	"context"
	"log"
	"time"

	"github.com/coreos/etcd/clientv3"
	"github.com/coreos/etcd/clientv3/naming"
	"google.golang.org/grpc"
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

	resolver := &naming.GRPCResolver{Client: client}
	balancer := grpc.RoundRobin(resolver)

	conn, err := grpc.Dial("/chapter4/greeter", grpc.WithBalancer(balancer),
		grpc.WithInsecure(), grpc.WithBlock())
	if err != nil {
		log.Fatal(err)
	}
	defer conn.Close()
	c := NewGreeterClient(conn)

	r, err := c.SayHello(context.TODO(), &HelloRequest{Name: "World!"})
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("Greeting: %s", r.Message)
}
