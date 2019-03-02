
all: pdf

pdf:
	docker run --rm -v `pwd`/:/work -u $(shell id -u):$(shell id -g) kauplan/review2.5 /bin/sh -c "cd /work/articles; rake preproc; rake pdf"

