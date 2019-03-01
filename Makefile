
all: pdf

pdf:
	docker run --rm -v `pwd`/articles:/work -u $(shell id -u):$(shell id -g) vvakame/review:3.0 /bin/sh -c "cd /work && review-pdfmaker config.yml"

#init:
#	docker run --rm -v `pwd`/articles:/work -u $(shell id -u):$(shell id -g) vvakame/review:3.0 /bin/sh -c "cd /work && review-init etcd-book"

preproc:
	# docker run --rm -v `pwd`/:/work -u $(shell id -u):$(shell id -g) vvakame/review:3.0 /bin/sh -c "cd /work/articles && review-preproc --replace etcd-book.re"
	docker run --rm -v `pwd`/:/work -u $(shell id -u):$(shell id -g) vvakame/review:3.0 /bin/sh -c "cd /work/articles && rake preproc"