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
package janus

import (
	"context"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"math/rand"
	"strings"
	"unicode"
)

func Parse(ctx context.Context) {
}

func newTransactionID() string {
	sb := strings.Builder{}
	for i := 0; i < 12; i++ {
		sb.WriteByte(byte('a') + byte(rand.Int()%26))
	}
	return sb.String()
}

func escapeJSON(s string) string {
	return strings.Map(func(r rune) rune {
		if unicode.IsSpace(r) {
			return rune(-1)
		}
		return r
	}, s)
}

func Run(ctx context.Context) error {
	ctx = logger.WithContext(ctx)
	r := "janus://localhost:8080/1234/livestream"

	api := newJanusAPI(r)
	if err := api.Create(ctx); err != nil {
		return errors.Wrapf(err, "create")
	}

	if err := api.JoinAsPublisher(ctx, 1234, "livestream"); err != nil {
		return errors.Wrapf(err, "join as publisher")
	}

	<-ctx.Done()

	return nil
}
