.PHONY: help default bench test

default: bench

bench: ./objs/srs_bench

./objs/srs_bench: *.go
	gofmt -w .
	go build -mod=vendor -o objs/srs_bench .

test:
	go test ./srs

help:
	@echo "Usage: make [bench|test]"
	@echo "     bench       Make the bench at ./objs/srs_bench"
	@echo "     test        Run all regression tests"
