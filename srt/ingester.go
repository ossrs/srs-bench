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
	"os"
	"time"

	"github.com/haivision/srtgo"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/pion/webrtc/v3/pkg/media/h264reader"
	"github.com/yapingcat/gomedia/go-mpeg2"
)

type tsIngester struct {
	srtSocket   *srtgo.SrtSocket
	stream      string
	sourceAudio string
	sourceVideo string
	avcSamples  uint64
	aacSamples  uint64
	fps         int
	cancel      context.CancelFunc
	ft          *os.File
}

func NewIngester(s *srtgo.SrtSocket, sm, sa, sv string, f int) *tsIngester {
	return &tsIngester{
		srtSocket:   s,
		stream:      sm,
		sourceAudio: sa,
		sourceVideo: sv,
		avcSamples:  0,
		aacSamples:  0,
		fps:         f,
	}
}

func (v *tsIngester) Ingest(ctx context.Context) error {
	ctx, v.cancel = context.WithCancel(ctx)

	vf, err := os.Open(v.sourceVideo)
	if err != nil {
		return errors.Wrapf(err, "Open source video %v", v.sourceVideo)
	}
	defer vf.Close()

	af, err := os.Open(v.sourceAudio)
	if err != nil {
		return errors.Wrapf(err, "open source audio %v", v.sourceAudio)
	}
	defer af.Close()

	h264, err := h264reader.NewReader(vf)
	if err != nil {
		return errors.Wrapf(err, "Open h264 failed, err=%v", v.sourceVideo)
	}

	audio, err := NewAACReader(af)
	if err != nil {
		return errors.Wrapf(err, "Open aac failed, err= %v", v.sourceAudio)
	}

	var srtError error = nil

	tsPacket := []byte{}
	tsMuxer := mpeg2.NewTSMuxer()
	tsMuxer.OnPacket = func(pkt []byte) {
		tsPacket = append(tsPacket, pkt...)

		// Merge ts packet
		if len(tsPacket) == 1316 {
			v.srtSocket.SetWriteDeadline(time.Now().Add(time.Duration(2 * time.Second)))
			if _, srtError = v.srtSocket.Write(tsPacket); srtError != nil {
				logger.Ef(ctx, "SRT write failed, err=%v", srtError)
				return
			}
			tsPacket = []byte{}
		}
	}

	audioPid := tsMuxer.AddStream(mpeg2.TS_STREAM_AAC)
	videoPid := tsMuxer.AddStream(mpeg2.TS_STREAM_H264)

	audioSampleRate := audio.codec.ASC().SampleRate.ToHz()
	videoSampleRate := 1024 * 1000 / v.fps

	var audioDts, videoDts, prevDts uint64
	lastPrint := time.Now()

	for ctx.Err() == nil {
		if srtError != nil {
			return errors.Wrapf(srtError, "SRT error")
		}

		if time.Now().Sub(lastPrint) > 5*time.Second {
			lastPrint = time.Now()
			logger.Tf(ctx, "Stream=%v, consume Video(samples=%v, dts=%v, ts=%.3f) and Audio(samples=%v, dts=%v, ts=%.3f)",
				v.stream, v.avcSamples, videoDts, float64(videoDts)/1000.0, v.aacSamples, audioDts, float64(audioDts)/1000.0)
		}

		frame, err := h264.NextNAL()
		if err == io.EOF {
			return io.EOF
		}
		if err != nil {
			return errors.Wrapf(err, "Read h264")
		}

		v.avcSamples += 1024
		videoDts = uint64(1000*v.avcSamples) / uint64(videoSampleRate)

		if prevDts == 0 {
			prevDts = videoDts
		} else if prevDts > 0 && videoDts > prevDts {
			// Make sure the video frame sent in correct interval.
			time.Sleep(time.Duration(videoDts-prevDts) * time.Millisecond)
			prevDts = videoDts
		}

		if err := tsMuxer.Write(videoPid, append([]byte{0, 0, 0, 1}, frame.Data...), videoDts, videoDts); err != nil {
			return errors.Wrapf(err, "Mux ts video")
		}

		for {
			audioFrame, err := audio.NextADTSFrame()
			if err != nil {
				return errors.Wrap(err, "Read AAC")
			}

			v.aacSamples += 1024
			audioDts = uint64(1000*v.aacSamples) / uint64(audioSampleRate)

			done := false

			if prevDts == 0 {
				prevDts = audioDts
			} else if prevDts > 0 && audioDts > prevDts {
				// Make sure the audio frame sent in correct interval.
				time.Sleep(time.Duration(audioDts-prevDts) * time.Millisecond)
				prevDts = audioDts

				// Mark we had send all the audio frame which dts is less then the prev video frame.
				done = true
			}

			if err := tsMuxer.Write(audioPid, audioFrame, audioDts, audioDts); err != nil {
				return errors.Wrapf(err, "Mux ts audio")
			}

			if done {
				break
			}
		}
	}

	return ctx.Err()
}

func (v *tsIngester) Close() error {
	if v.ft != nil {
		v.ft.Close()
	}
	if v.cancel != nil {
		v.cancel()
	}
	return nil
}
