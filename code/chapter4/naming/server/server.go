//go:generate protoc -I ../pb --go_out=plugins=grpc:./ ../pb/helloworld.proto
package main

import (
	"context"
	"log"
	"net"
	"time"

	"github.com/coreos/etcd/clientv3"
	"github.com/coreos/etcd/clientv3/naming"
	"google.golang.org/grpc"
	gn "google.golang.org/grpc/naming"
)

type greeter struct{}

func (s *greeter) SayHello(ctx context.Context, in *HelloRequest) (*HelloReply, error) {
	log.Printf("Received: %v", in.Name)
	return &HelloReply{Message: "Hello " + in.Name}, nil
}

func main() {
	client, err := clientv3.New(clientv3.Config{
		Endpoints:   []string{"http://localhost:2379"},
		DialTimeout: 3 * time.Second,
	})
	if err != nil {
		log.Fatal(err)
	}
	defer client.Close()

	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		log.Fatal(err)
	}

	resolver := &naming.GRPCResolver{Client: client}
	err = resolver.Update(context.TODO(), "/chapter4/greeter",
		gn.Update{Op: gn.Add, Addr: listener.Addr().String()})
	if err != nil {
		log.Fatal(err)
	}

	server := grpc.NewServer()
	RegisterGreeterServer(server, &greeter{})
	if err := server.Serve(listener); err != nil {
		log.Fatal(err)
	}
}
