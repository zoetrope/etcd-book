//go:generate protoc -I ../proto --go_out=plugins=grpc:./ ../proto/helloworld.proto

// Package main implements a server for Greeter service.
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

const (
	port = ":50051"
)

// server is used to implement helloworld.GreeterServer.
type server struct{}

// SayHello implements helloworld.GreeterServer
func (s *server) SayHello(ctx context.Context, in *HelloRequest) (*HelloReply, error) {
	log.Printf("Received: %v", in.Name)
	return &HelloReply{Message: "Hello " + in.Name}, nil
}

func main() {

	err := registerService()
	if err != nil {
		log.Fatal(err)
	}

	lis, err := net.Listen("tcp", port)
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}
	s := grpc.NewServer()
	RegisterGreeterServer(s, &server{})
	if err := s.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}

func registerService() error {
	cfg := clientv3.Config{
		Endpoints:   []string{"http://localhost:2379"},
		DialTimeout: 3 * time.Second,
	}

	client, err := clientv3.New(cfg)
	if err != nil {
		return err
	}
	defer client.Close()

	r := &naming.GRPCResolver{Client: client}
	err = r.Update(context.TODO(), "helloworld", gn.Update{Op: gn.Add, Addr: "127.0.0.1" + port})
	return err
}
