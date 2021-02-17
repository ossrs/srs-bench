package main

import (
	"context"
	"fmt"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/pion/rtcp"
	"github.com/pion/webrtc/v3"
	"github.com/pion/webrtc/v3/pkg/media"
	"github.com/pion/webrtc/v3/pkg/media/h264writer"
	"github.com/pion/webrtc/v3/pkg/media/ivfwriter"
	"github.com/pion/webrtc/v3/pkg/media/oggwriter"
	"strings"
	"time"
)

// @see https://github.com/pion/webrtc/blob/master/examples/save-to-disk/main.go
func startPlay(ctx context.Context, r, dumpAudio, dumpVideo string) error {
	ctx = logger.WithContext(ctx)

	logger.Tf(ctx, "Start play url=%v, audio=%v, video=%v", r, dumpAudio, dumpVideo)

	pc, err := webrtc.NewPeerConnection(webrtc.Configuration{})
	if err != nil {
		return errors.Wrapf(err, "Create PC")
	}
	defer pc.Close()

	pc.AddTransceiverFromKind(webrtc.RTPCodecTypeAudio, webrtc.RTPTransceiverInit{
		Direction: webrtc.RTPTransceiverDirectionRecvonly,
	})
	pc.AddTransceiverFromKind(webrtc.RTPCodecTypeVideo, webrtc.RTPTransceiverInit{
		Direction: webrtc.RTPTransceiverDirectionRecvonly,
	})

	offer, err := pc.CreateOffer(nil)
	if err != nil {
		return errors.Wrapf(err, "Create Offer")
	}

	if err := pc.SetLocalDescription(offer); err != nil {
		return errors.Wrapf(err, "Set offer %v", offer)
	}

	answer, err := apiRtcRequest(ctx, "/rtc/v1/play", r, offer.SDP)
	if err != nil {
		return errors.Wrapf(err, "Api request offer=%v", offer.SDP)
	}

	if err := pc.SetRemoteDescription(webrtc.SessionDescription{
		Type: webrtc.SDPTypeAnswer, SDP: answer,
	}); err != nil {
		return errors.Wrapf(err, "Set answer %v", answer)
	}

	var da media.Writer
	var dv_vp8 media.Writer
	var dv_h264 media.Writer
	defer func() {
		if da != nil {
			da.Close()
		}
		if dv_vp8 != nil {
			dv_vp8.Close()
		}
		if dv_h264 != nil {
			dv_h264.Close()
		}
	}()

	handleTrack := func(ctx context.Context, track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) error {
		// Send a PLI on an interval so that the publisher is pushing a keyframe
		go func() {
			if track.Kind() == webrtc.RTPCodecTypeAudio {
				return
			}

			ticker := time.NewTicker(3 * time.Second)
			for {
				select {
				case <-ctx.Done():
					return
				case <-ticker.C:
					_ = pc.WriteRTCP([]rtcp.Packet{&rtcp.PictureLossIndication{
						MediaSSRC: uint32(track.SSRC()),
					}})
				}
			}
		}()

		codec := track.Codec()

		trackDesc := fmt.Sprintf("channels=%v", codec.Channels)
		if track.Kind() == webrtc.RTPCodecTypeVideo {
			trackDesc = fmt.Sprintf("fmtp=%v", codec.SDPFmtpLine)
		}
		logger.Tf(ctx, "Got track %v, pt=%v, tbn=%v, %v",
			codec.MimeType, codec.PayloadType, codec.ClockRate, trackDesc)

		if codec.MimeType == "audio/opus" {
			if da == nil && dumpAudio != "" {
				if da, err = oggwriter.New(dumpAudio, codec.ClockRate, codec.Channels); err != nil {
					return errors.Wrapf(err, "New audio dumper")
				}
				logger.Tf(ctx, "Open ogg writer file=%v, tbn=%v, channels=%v",
					dumpAudio, codec.ClockRate, codec.Channels)
			}

			if err = writeTrackToDisk(ctx, da, track); err != nil {
				return errors.Wrapf(err, "Write audio disk")
			}
		} else if codec.MimeType == "video/VP8" {
			if dumpVideo != "" && !strings.HasSuffix(dumpVideo, ".ivf") {
				return errors.Errorf("%v should be .ivf for VP8", dumpVideo)
			}

			if dv_vp8 == nil && dumpVideo != "" {
				if dv_vp8, err = ivfwriter.New(dumpVideo); err != nil {
					return errors.Wrapf(err, "New video dumper")
				}
				logger.Tf(ctx, "Open ivf writer file=%v", dumpVideo)
			}

			if err = writeTrackToDisk(ctx, dv_vp8, track); err != nil {
				return errors.Wrapf(err, "Write video disk")
			}
		} else if codec.MimeType == "video/H264" {
			if dumpVideo != "" && !strings.HasSuffix(dumpVideo, ".h264") {
				return errors.Errorf("%v should be .h264 for H264", dumpVideo)
			}

			if dv_h264 == nil && dumpVideo != "" {
				if dv_h264, err = h264writer.New(dumpVideo); err != nil {
					return errors.Wrapf(err, "New video dumper")
				}
				logger.Tf(ctx, "Open h264 writer file=%v", dumpVideo)
			}

			if err = writeTrackToDisk(ctx, dv_h264, track); err != nil {
				return errors.Wrapf(err, "Write video disk")
			}
		} else {
			logger.Wf(ctx, "Ignore track %v pt=%v", codec.MimeType, codec.PayloadType)
		}
		return nil
	}

	ctx, cancel := context.WithCancel(ctx)
	pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
		err = handleTrack(ctx, track, receiver)
		if err != nil {
			codec := track.Codec()
			err = errors.Wrapf(err, "Handle  track %v, pt=%v", codec.MimeType, codec.PayloadType)
			cancel()
		}
	})

	pc.OnICEConnectionStateChange(func(state webrtc.ICEConnectionState) {
		logger.If(ctx, "ICE state %v", state)

		if state == webrtc.ICEConnectionStateFailed || state == webrtc.ICEConnectionStateClosed {
			if ctx.Err() != nil {
				return
			}

			logger.Wf(ctx, "Close for ICE state %v", state)
			cancel()
		}
	})

	<-ctx.Done()
	return err
}

func writeTrackToDisk(ctx context.Context, w media.Writer, track *webrtc.TrackRemote) error {
	for ctx.Err() == nil {
		pkt, _, err := track.ReadRTP()
		if err != nil {
			return errors.Wrapf(err, "Read RTP")
		}

		if w == nil {
			continue
		}

		if err := w.WriteRTP(pkt); err != nil {
			if len(pkt.Payload) <= 2 {
				continue
			}
			logger.Wf(ctx, "Ignore write RTP %vB err %+v", len(pkt.Payload), err)
		}
	}

	return ctx.Err()
}
