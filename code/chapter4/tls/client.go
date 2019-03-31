package main

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io/ioutil"
	"log"
	"time"

	"github.com/coreos/etcd/clientv3"
)

func main() {
	//#@@range_begin(tls)
	tlsCfg := &tls.Config{}
	rootCACert, err := ioutil.ReadFile("./certs/ca.pem")
	if err != nil {
		log.Fatal(err)
	}
	rootCAs := x509.NewCertPool()
	ok := rootCAs.AppendCertsFromPEM(rootCACert)
	if !ok {
		log.Fatal("failed to parse pem file")
	}
	tlsCfg.RootCAs = rootCAs

	cert, err := tls.LoadX509KeyPair("./certs/client.pem", "./certs/client-key.pem")
	if err != nil {
		log.Fatal(err)
	}
	tlsCfg.Certificates = []tls.Certificate{cert}
	//#@@range_end(tls)

	//#@@range_begin(client)
	client, err := clientv3.New(clientv3.Config{
		Endpoints:   []string{"http://localhost:2379"},
		DialTimeout: 3 * time.Second,
		TLS:         tlsCfg,
	})
	//#@@range_end(client)
	if err != nil {
		log.Fatal(err)
	}
	defer client.Close()

	resp, err := client.MemberList(context.TODO())
	if err != nil {
		log.Fatal(err)
	}
	for _, m := range resp.Members {
		fmt.Printf("%s\n", m.String())
	}
}
