// The MIT License (MIT)
//
// # Copyright (c) 2022 John
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
package srt

import (
	"context"
	"io"
	"net"
	"net/url"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/haivision/srtgo"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
)

func startSrtPublish(ctx context.Context, pr string, sourceAudio, sourceVideo string, fps int) error {
	u, err := url.Parse(pr)
	if err != nil {
		return errors.Wrapf(err, "Parse url")
	}

	addr := u.Host
	if !strings.Contains(addr, ":") {
		addr += ":10080"
	}

	ip, p, err := net.SplitHostPort(addr)
	if err != nil {
		return errors.Wrapf(err, "Invalid addr=%v", addr)
	}

	port, _ := strconv.Atoi(p)

	stream := u.Path
	if len(stream) > 0 && stream[0] == '/' {
		stream = stream[1:]
	}

	srtSocket := srtgo.NewSrtSocket(ip, uint16(port),
		map[string]string{
			"transtype": "live",
			"tsbpdmode": "false",
			"tlpktdrop": "false",
			"latency":   "0",
			"streamid":  "#!::r=" + stream + ",m=publish",
		},
	)
	defer srtSocket.Close()

	srtSocket.SetWriteDeadline(time.Now().Add(time.Duration(2 * time.Second)))
	if err := srtSocket.Connect(); err != nil {
		return errors.Wrapf(err, "SRT connect to %v:%v", ip, port)
	}

	ingester := NewIngester(srtSocket, stream, sourceAudio, sourceVideo, fps)
	defer ingester.Close()

	var wg sync.WaitGroup

	wg.Add(1)
	go func() {
		defer wg.Done()

		if ingester == nil {
			return
		}

		for ctx.Err() == nil {
			if err := ingester.Ingest(ctx); err != nil {
				if errors.Cause(err) == io.EOF {
					logger.Tf(ctx, "Stream=%v EOF, restart ingest", stream)
					continue
				}

				logger.Ef(ctx, "Ingest failed, err=%v", err)
				break
			}
		}
	}()

	wg.Wait()
	return nil
}
