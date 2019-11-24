
all: pdf

pdf:
	docker run --rm -v `pwd`/:/work -u $(shell id -u):$(shell id -g) kauplan/review2.5 /bin/sh -c "cd /work/articles; rake preproc; rake pdf"

preproc:
	docker run --rm -v `pwd`/:/work -u $(shell id -u):$(shell id -g) kauplan/review2.5 /bin/sh -c "cd /work/articles; rake preproc"

lint:
	npm run textlint

start-etcd:
	docker run --name etcd \
		-p 2379:2379 \
		--volume=etcd-data:/etcd-data \
    	--name etcd gcr.io/etcd-development/etcd:v3.3.17 \
		/usr/local/bin/etcd \
			--name=etcd-1 \
			--data-dir=/etcd-data \
			--advertise-client-urls http://0.0.0.0:2379 \
			--listen-client-urls http://0.0.0.0:2379 \
			--enable-pprof=true

destroy:
	docker stop etcd
	docker rm etcd
	docker volume rm etcd-data
