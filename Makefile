
all: pdf

pdf:
	docker run --rm -v `pwd`/:/work -u $(shell id -u):$(shell id -g) kauplan/review2.5 /bin/sh -c "cd /work/articles; rake preproc; rake pdf"

preproc:
	docker run --rm -v `pwd`/:/work -u $(shell id -u):$(shell id -g) kauplan/review2.5 /bin/sh -c "cd /work/articles; rake preproc"

start-etcd:
	docker run --rm -d -p 2379:2379 --name etcd -v etcd-data:/var/lib/etcd quay.io/cybozu/etcd:3.3 --advertise-client-urls http://0.0.0.0:2379 --listen-client-urls http://0.0.0.0:2379
