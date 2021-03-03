.PHONY: help default bench test

default: bench

bench: ./objs/srs_bench

./objs/srs_bench: *.go rtc/*.go srs/*.go
	gofmt -w .
	go build -mod=vendor -o objs/srs_bench .

test: ./objs/srs_test

./objs/srs_test: *.go rtc/*.go srs/*.go
	go test ./srs -o ./objs/srs_test

help:
	@echo "Usage: make [bench|test]"
	@echo "     bench       Make the bench to ./objs/srs_bench"
	@echo "     test        Make the test tool to ./objs/srs_test"
