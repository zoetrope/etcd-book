package main

import (
	"context"
	"fmt"
	"log"
	"math/rand"
	"strconv"
	"time"

	"github.com/coreos/etcd/clientv3"
	"github.com/coreos/etcd/clientv3/concurrency"
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
	// fuzzyRead(client)
	phantomRead(client)
}

func fuzzyRead(client *clientv3.Client) {
	//#@@range_begin(fuzzy)
	addValue := func(d int) {
		_, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
			v1 := stm.Get("/chapter3/iso/fuzzy")
			value, err := strconv.Atoi(v1)
			if err != nil {
				return err
			}
			value += d
			time.Sleep(time.Duration(rand.Intn(3)) * time.Second)
			v2 := stm.Get("/chapter3/iso/fuzzy")
			if v1 != v2 {
				// ReadCommittedが正しく実装されているならここでファジーリードが発生するはず。
				// しかしetcd v3.3.12ではちゃんと実装されてない
				fmt.Printf("fuzzy:%d, %s, %s\n", d, v1, v2)
			}
			stm.Put("/chapter3/iso/fuzzy", strconv.Itoa(value))
			return nil
		}, concurrency.WithIsolation(concurrency.ReadCommitted))
		if err != nil {
			log.Fatal(err)
		}
	}

	client.Put(context.TODO(), "/chapter3/iso/fuzzy", "10")
	go addValue(5)
	go addValue(-3)

	time.Sleep(5 * time.Second)
	// トランザクションにもなっていないので、結果は7になったり15になったりばらつく。
	resp, _ := client.Get(context.TODO(), "/chapter3/iso/fuzzy")
	fmt.Println(string(resp.Kvs[0].Value))
	//#@@range_end(fuzzy)
}

func phantomRead(client *clientv3.Client) {
	//#@@range_begin(phantom)
	addValue := func(d int) {
		_, err := concurrency.NewSTM(client, func(stm concurrency.STM) error {
			v1 := stm.Get("/chapter3/iso/phantom/key1")
			value := 0
			if len(v1) != 0 {
				value, _ = strconv.Atoi(v1)
			}
			value += d
			time.Sleep(time.Duration(rand.Intn(3)) * time.Second)
			v2 := stm.Get("/chapter3/iso/phantom/key2")
			if v1 != v2 {
				// key1とkey2の値は本来は同じはずである。
				// しかし読み取るタイミングが異なったせいで異なる値が入ってしまうケースがある。
				// これがファントムリード
				fmt.Printf("phantom:%d, %s, %s\n", d, v1, v2)
			}
			stm.Put("/chapter3/iso/phantom/key1", strconv.Itoa(value))
			stm.Put("/chapter3/iso/phantom/key2", strconv.Itoa(value))
			return nil
		}, concurrency.WithIsolation(concurrency.RepeatableReads)) // RepeatableReadsを指定したときだけファントムリードが発生する
		if err != nil {
			log.Fatal(err)
		}
	}

	client.Delete(context.TODO(), "/chapter3/iso/phantom/", clientv3.WithPrefix())
	go addValue(5)
	go addValue(-3)

	time.Sleep(5 * time.Second)
	resp, _ := client.Get(context.TODO(), "/chapter3/iso/phantom/key1")
	fmt.Println(string(resp.Kvs[0].Value))
	//#@@range_end(phantom)
}
