// The MIT License (MIT)
//
// Copyright (c) 2022 Winlin
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
package gb28181

import (
	"context"
	"flag"
	"fmt"
	"github.com/ghettovoice/gosip/sip"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/pion/webrtc/v3/pkg/media/h264reader"
	"io"
	"os"
	"strconv"
	"strings"
	"time"
)

var sipConfig SIPConfig
var psConfig PSConfig

func Parse(ctx context.Context) {
	fl := flag.NewFlagSet(os.Args[0], flag.ContinueOnError)

	var sfu string
	fl.StringVar(&sfu, "sfu", "srs", "The SFU server, srs or gb28181 or janus")

	fl.StringVar(&sipConfig.addr, "pr", "", "")
	fl.StringVar(&sipConfig.user, "user", "", "")
	fl.StringVar(&sipConfig.server, "server", "", "")
	fl.StringVar(&sipConfig.domain, "domain", "", "")
	fl.IntVar(&sipConfig.random, "random", 0, "")

	fl.StringVar(&psConfig.video, "sv", "", "")
	fl.StringVar(&psConfig.audio, "sa", "", "")
	fl.IntVar(&psConfig.fps, "fps", 0, "")

	fl.Usage = func() {
		fmt.Println(fmt.Sprintf("Usage: %v [Options]", os.Args[0]))
		fmt.Println(fmt.Sprintf("Options:"))
		fmt.Println(fmt.Sprintf("   -sfu    The target SFU, srs or gb28181 or janus. Default: srs"))
		fmt.Println(fmt.Sprintf("SIP:"))
		fmt.Println(fmt.Sprintf("   -user   The SIP username, ID of device."))
		fmt.Println(fmt.Sprintf("   -random Append N number to user as random device ID, like 1320000001."))
		fmt.Println(fmt.Sprintf("   -server The SIP server ID, ID of server."))
		fmt.Println(fmt.Sprintf("   -domain The SIP domain, domain of server and device."))
		fmt.Println(fmt.Sprintf("Publisher:"))
		fmt.Println(fmt.Sprintf("   -pr     The SIP server address, format is tcp://ip:port over TCP."))
		fmt.Println(fmt.Sprintf("   -fps    [Optional] The fps of .h264 source file."))
		fmt.Println(fmt.Sprintf("   -sa     [Optional] The file path to read audio, ignore if empty."))
		fmt.Println(fmt.Sprintf("   -sv     [Optional] The file path to read video, ignore if empty."))
		fmt.Println(fmt.Sprintf("\n例如，1个推流："))
		fmt.Println(fmt.Sprintf("   %v -sfu gb28181 -pr tcp://127.0.0.1:5060 -user 34020000001320000001 -server 34020000002000000001 -domain 3402000000", os.Args[0]))
		fmt.Println(fmt.Sprintf("   %v -sfu gb28181 -pr tcp://127.0.0.1:5060 -user 3402000000 -random 10 -server 34020000002000000001 -domain 3402000000", os.Args[0]))
		fmt.Println(fmt.Sprintf("   %v -sfu gb28181 -pr tcp://127.0.0.1:5060 -user 3402000000 -random 10 -server 34020000002000000001 -domain 3402000000 -sa avatar.aac -sv avatar.h264 -fps 25", os.Args[0]))
		fmt.Println(fmt.Sprintf("   %v -sfu gb28181 -pr tcp://127.0.0.1:5060 -user livestream -server srs -domain ossrs.io -sa avatar.aac -sv avatar.h264 -fps 25", os.Args[0]))
		fmt.Println()
	}
	if err := fl.Parse(os.Args[1:]); err == flag.ErrHelp {
		os.Exit(0)
	}

	showHelp := sipConfig.String() == ""
	if showHelp {
		fl.Usage()
		os.Exit(-1)
	}

	summaryDesc := ""
	if sipConfig.addr != "" {
		pubString := strings.Join([]string{sipConfig.String(), psConfig.String()}, ",")
		summaryDesc = fmt.Sprintf("%v, publish(%v)", summaryDesc, pubString)
	}
	logger.Tf(ctx, "Run benchmark with %v", summaryDesc)
}

func Run(ctx context.Context) error {
	ctx, cancel := context.WithCancel(ctx)
	regTimeout := 3 * time.Hour
	inviteTimeout := 3 * time.Hour

	client := NewSIPSession(&sipConfig)
	if err := client.Connect(ctx); err != nil {
		return errors.Wrap(err, "connect")
	}
	defer client.Close()

	for {
		ctx, regCancel := context.WithTimeout(ctx, regTimeout)
		defer regCancel()

		regReq, regRes, err := client.Register(ctx)
		if err != nil {
			return errors.Wrap(err, "register")
		}
		logger.Tf(ctx, "Register id=%v, response=%v", regReq.MessageID(), regRes.MessageID())

		break
	}

	var ssrc int64
	var mediaPort int64
	clockRate, payloadType := uint64(90000), 96
	for {
		ctx, inviteCancel := context.WithTimeout(ctx, inviteTimeout)
		defer inviteCancel()

		inviteReq, err := client.Wait(ctx, sip.INVITE)
		if err != nil {
			return errors.Wrap(err, "wait")
		}

		if err = client.Trying(ctx, inviteReq); err != nil {
			return errors.Wrapf(err, "trying invite is %v", inviteReq.String())
		}

		inviteRes, err := client.InviteResponse(ctx, inviteReq)
		if err != nil {
			return errors.Wrapf(err, "response invite is %v", inviteReq.String())
		}

		offer := inviteReq.Body()
		ssrcStr := strings.Split(strings.Split(offer, "y=")[1], "\r\n")[0]
		if ssrc, err = strconv.ParseInt(ssrcStr, 10, 64); err != nil {
			return errors.Wrapf(err, "parse ssrc=%v, sdp %v", ssrcStr, offer)
		}
		mediaPortStr := strings.Split(strings.Split(offer, "m=video")[1], " ")[1]
		if mediaPort, err = strconv.ParseInt(mediaPortStr, 10, 64); err != nil {
			return errors.Wrapf(err, "parse media port=%v, sdp %v", mediaPortStr, offer)
		}
		logger.Tf(ctx, "Invite id=%v, response=%v, y=%v, ssrc=%v, mediaPort=%v",
			inviteReq.MessageID(), inviteRes.MessageID(), ssrcStr, ssrc, mediaPort,
		)

		break
	}

	if psConfig.video == "" || psConfig.audio == "" {
		cancel()
		return nil
	}

	ps := NewPSClient(uint32(ssrc), int(mediaPort))
	if err := ps.Connect(ctx); err != nil {
		return errors.Wrapf(err, "connect media=%v", mediaPort)
	}
	defer ps.Close()

	videoFile, err := os.Open(psConfig.video)
	if err != nil {
		return errors.Wrapf(err, "Open file %v", psConfig.video)
	}
	defer videoFile.Close()

	f, err := os.Open(psConfig.audio)
	if err != nil {
		return errors.Wrapf(err, "Open file %v", psConfig.audio)
	}
	defer f.Close()

	h264, err := h264reader.NewReader(videoFile)
	if err != nil {
		return errors.Wrapf(err, "Open h264 %v", psConfig.video)
	}

	audio, err := NewAACReader(f)
	if err != nil {
		return errors.Wrapf(err, "Open ogg %v", psConfig.audio)
	}

	// Scale the video samples to 1024 according to AAC, that is 1 video frame means 1024 samples.
	audioSampleRate := audio.codec.ASC().SampleRate.ToHz()
	videoSampleRate := 1024 * 1000 / psConfig.fps
	logger.Tf(ctx, "PS: Media stream, tbn=%v, ssrc=%v, pt=%v, Video(%v, fps=%v, rate=%v), Audio(%v, rate=%v, channels=%v)",
		clockRate, ssrc, payloadType, psConfig.video, psConfig.fps, videoSampleRate, psConfig.audio, audioSampleRate, audio.codec.ASC().Channels)

	clock := newWallClock()
	var aacSamples, avcSamples uint64
	var audioDTS, videoDTS uint64
	var pack *PSPackStream
	for ctx.Err() == nil {
		if pack == nil {
			pack = NewPSPackStream()
		}

		// One pack should only contains one video frame.
		if !pack.hasVideo {
			var sps, pps *h264reader.NAL
			var videoFrames []*h264reader.NAL
			for ctx.Err() == nil {
				frame, err := h264.NextNAL()
				if err == io.EOF {
					return io.EOF
				}
				if err != nil {
					return errors.Wrapf(err, "Read h264")
				}

				videoFrames = append(videoFrames, frame)
				logger.If(ctx, "NALU %v PictureOrderCount=%v, ForbiddenZeroBit=%v, RefIdc=%v, %v bytes",
					frame.UnitType.String(), frame.PictureOrderCount, frame.ForbiddenZeroBit, frame.RefIdc, len(frame.Data))

				if frame.UnitType == h264reader.NalUnitTypeSPS {
					sps = frame
				} else if frame.UnitType == h264reader.NalUnitTypePPS {
					pps = frame
				} else {
					break
				}
			}

			// We convert the video sample rate to be based over 1024, that is 1024 samples means one video frame.
			avcSamples += 1024
			videoDTS = uint64(clockRate*avcSamples) / uint64(videoSampleRate)

			if sps != nil || pps != nil {
				err = pack.WriteHeader(videoDTS)
			} else {
				err = pack.WritePackHeader(videoDTS)
			}
			if err != nil {
				return errors.Wrap(err, "pack header")
			}

			for _, frame := range videoFrames {
				if err = pack.WriteVideo(frame.Data, videoDTS); err != nil {
					return errors.Wrapf(err, "write video %v", len(frame.Data))
				}
			}
		}

		// Always read and consume one audio frame each time.
		if true {
			audioFrame, err := audio.NextADTSFrame()
			if err != nil {
				return errors.Wrap(err, "Read AAC")
			}

			// Each AAC frame contains 1024 samples, DTS = total-samples / sample-rate
			aacSamples += 1024
			audioDTS = uint64(clockRate*aacSamples) / uint64(audioSampleRate)
			logger.If(ctx, "Consume Video(samples=%v, dts=%v, ts=%.2f) and Audio(samples=%v, dts=%v, ts=%.2f)",
				avcSamples, videoDTS, float64(videoDTS)/90.0, aacSamples, audioDTS, float64(audioDTS)/90.0,
			)

			if err = pack.WriteAudio(audioFrame, audioDTS); err != nil {
				return errors.Wrapf(err, "write audio %v", len(audioFrame))
			}
		}

		if pack.hasVideo && videoDTS < audioDTS {
			if err := ps.WritePacksOverRTP(pack.packets); err != nil {
				return errors.Wrap(err, "write")
			}
			pack = nil // Reset pack.
		}

		// One audio frame(1024 samples), the duration is 1024/audioSampleRate in seconds.
		sampleDuration := time.Duration(uint64(time.Second) * 1024 / uint64(audioSampleRate))
		if d := clock.Tick(sampleDuration); d > 0 {
			time.Sleep(d)
		}
	}

	return nil
}
