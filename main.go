// The MIT License (MIT)
//
// Copyright (c) 2021 Winlin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
package main

import (
	"context"
	"flag"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/ossrs/srs-bench/srs"
	"os"
	"os/signal"
	"syscall"
)

func main() {
	var sfu string
	fl := flag.NewFlagSet(os.Args[0], flag.ContinueOnError)
	fl.StringVar(&sfu, "sfu", "srs", "The SFU server, srs or janus")
	_ = fl.Parse(os.Args[1:])

	ctx := context.Background()
	if sfu == "srs" {
		srs.Parse(ctx)
	} else {
		logger.Ef(ctx, "invalid sfu %v", sfu)
		os.Exit(-1)
	}

	ctx, cancel := context.WithCancel(ctx)
	go func() {
		sigs := make(chan os.Signal, 1)
		signal.Notify(sigs, syscall.SIGTERM, syscall.SIGINT)
		for sig := range sigs {
			logger.Wf(ctx, "Quit for signal %v", sig)
			cancel()
		}
	}()

	if sfu == "srs" {
		if err := srs.Start(ctx); err != nil {
			logger.Wf(ctx, "srs err %+v", err)
			return
		}
	}

	logger.Tf(ctx, "Done")
}
