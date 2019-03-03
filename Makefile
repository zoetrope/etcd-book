
all: pdf

pdf:
	docker run --rm -v `pwd`/:/work -u $(shell id -u):$(shell id -g) kauplan/review2.5 /bin/sh -c "cd /work/articles; rake preproc; rake pdf"

preproc:
	docker run --rm -v `pwd`/:/work -u $(shell id -u):$(shell id -g) kauplan/review2.5 /bin/sh -c "cd /work/articles; rake preproc"

start-etcd:
	docker run -d --name etcd quay.io/coreos/etcd:v3.3.12
