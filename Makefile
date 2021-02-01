.PHONY: default

default: ./objs/srs_bench

./objs/srs_bench: *.go
	gofmt -w .
	go build -mod=vendor -o objs/srs_bench .

