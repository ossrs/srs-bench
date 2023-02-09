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
	"bytes"
	"context"
	"net"
	"net/url"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/haivision/srtgo"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/yapingcat/gomedia/mpeg2"
)

func startSrtPlay(ctx context.Context, sr, da, dv, dt string) error {
	ctx = logger.WithContext(ctx)
	u, err := url.Parse(sr)
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

	srtgo.SrtSetLogHandler(func(level srtgo.SrtLogLevel, file string, line int, area, message string) {
		switch level {
		case srtgo.SrtLogLevelCrit:
		case srtgo.SrtLogLevelErr:
			logger.Ef(ctx, "%v:%v # %v", file, line, message)
		case srtgo.SrtLogLevelWarning:
			logger.Wf(ctx, "%v:%v # %v", file, line, message)
		case srtgo.SrtLogLevelNotice:
		case srtgo.SrtLogLevelInfo:
			logger.Tf(ctx, "%v:%v # %v", file, line, message)
		case srtgo.SrtLogLevelDebug:
			logger.Tf(ctx, "%v:%v # %v", file, line, message)
		case srtgo.SrtLogLevelTrace:
			logger.Tf(ctx, "%v:%v # %v", file, line, message)
		}
	})

	srtgo.SrtSetLogLevel(srtgo.SrtLogLevelTrace)

	srtSocket := srtgo.NewSrtSocket(ip, uint16(port),
		map[string]string{
			"transtype": "live",
			"tsbpdmode": "false",
			"tlpktdrop": "false",
			"latency":   "0",
			"streamid":  "#!::r=" + stream + ",m=request",
		},
	)

	defer srtSocket.Close()

	srtSocket.SetWriteDeadline(time.Now().Add(time.Duration(2 * time.Second)))
	if err := srtSocket.Connect(); err != nil {
		return errors.Wrapf(err, "SRT connect to %v:%v", ip, port)
	}

	var fa *os.File = nil
	if da != "" {
		if fa, err = os.OpenFile(da, os.O_TRUNC|os.O_CREATE|os.O_WRONLY, 0644); err != nil {
			return errors.Wrapf(err, "Open audio dump file %v", da)
		}
		defer fa.Close()
	}

	var fv *os.File = nil
	if dv != "" {
		if fv, err = os.OpenFile(dv, os.O_TRUNC|os.O_CREATE|os.O_WRONLY, 0644); err != nil {
			return errors.Wrapf(err, "Open video dump file %v", dv)
		}
		defer fv.Close()
	}

	var audioFrameNum, videoFrameNum uint64
	var audioDts, videoDts uint64
	lastPrint := time.Now()

	demuxer := mpeg2.NewTSDemuxer()
	demuxer.OnFrame = func(cid mpeg2.TS_STREAM_TYPE, frame []byte, pts uint64, dts uint64) {
		if cid == mpeg2.TS_STREAM_H264 || cid == mpeg2.TS_STREAM_H265 {
			videoFrameNum += 1
			videoDts = dts

			if fv != nil {
				fv.Write(frame)
			}
		} else if cid == mpeg2.TS_STREAM_AAC {
			audioFrameNum += 1
			audioDts = dts

			if fa != nil {
				fa.Write(frame)
			}
		}

		if time.Now().Sub(lastPrint) > 5*time.Second {
			lastPrint = time.Now()
			logger.Tf(ctx, "Stream=%v receive Video(frames=%v, dts=%v, ts=%.3f) and Audio(frames=%v, dts=%v, ts=%.3f)",
				stream, videoFrameNum, videoDts, float64(videoDts)/1000.0, audioFrameNum, audioDts, float64(audioDts)/1000.0)
		}
	}

	var ft *os.File = nil
	if dt != "" {
		if ft, err = os.OpenFile(dt, os.O_TRUNC|os.O_CREATE|os.O_WRONLY, 0644); err != nil {
			return errors.Wrapf(err, "Open ts dump file %v", dt)
		}
		defer ft.Close()
	}

	go func() {
		select {
		case <-ctx.Done():
			srtSocket.Close()
			logger.Tf(ctx, "ctx done, close srt socket")
		}
	}()

	buf := make([]byte, 1500)
	for ctx.Err() == nil {
		srtSocket.SetReadDeadline(time.Now().Add(time.Duration(2 * time.Second)))
		nb, err := srtSocket.Read(buf)
		if err != nil {
			return errors.Wrapf(err, "SRT read")
		}

		if ft != nil {
			ft.Write(buf[:nb])
		}

		if err := demuxer.Input(bytes.NewReader(buf[:nb])); err != nil {
			return errors.Wrapf(err, "Demux ts")
		}
	}
	return nil
}
