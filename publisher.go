package main

import (
	"context"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/pion/rtp"
	"github.com/pion/webrtc/v3"
	"github.com/pion/webrtc/v3/pkg/media"
	"github.com/pion/webrtc/v3/pkg/media/h264reader"
	"github.com/pion/webrtc/v3/pkg/media/oggreader"
	"io"
	"os"
	"strings"
	"sync"
	"time"
)

// @see https://github.com/pion/webrtc/blob/master/examples/play-from-disk/main.go
func startPublish(ctx context.Context, r, sourceAudio, sourceVideo string, fps int) error {
	ctx = logger.WithContext(ctx)

	logger.Tf(ctx, "Start publish url=%v, audio=%v, video=%v, fps=%v", r, sourceAudio, sourceVideo, fps)

	pc, err := webrtc.NewPeerConnection(webrtc.Configuration{})
	if err != nil {
		return errors.Wrapf(err, "Create PC")
	}

	var sVideoTrack *TrackLocalStaticSample
	var sVideoSender *webrtc.RTPSender
	if sourceVideo != "" {
		mimeType, trackID := "video/H264", "video"
		if strings.HasSuffix(sourceVideo, ".ivf") {
			mimeType = "video/VP8"
		}

		sVideoTrack, err = NewTrackLocalStaticSample(
			webrtc.RTPCodecCapability{MimeType: mimeType, ClockRate: 90000}, trackID, "pion",
		)
		if err != nil {
			return errors.Wrapf(err, "Create video track")
		}

		sVideoSender, err = pc.AddTrack(sVideoTrack)
		if err != nil {
			return errors.Wrapf(err, "Add video track")
		}
	}

	var sAudioTrack *TrackLocalStaticSample
	var sAudioSender *webrtc.RTPSender
	if sourceAudio != "" {
		mimeType, trackID := "audio/opus", "audio"
		sAudioTrack, err = NewTrackLocalStaticSample(
			webrtc.RTPCodecCapability{MimeType: mimeType, ClockRate: 48000, Channels: 2}, trackID, "pion",
		)
		if err != nil {
			return errors.Wrapf(err, "Create audio track")
		}

		sAudioSender, err = pc.AddTrack(sAudioTrack)
		if err != nil {
			return errors.Wrapf(err, "Add audio track")
		}
	}

	offer, err := pc.CreateOffer(nil)
	if err != nil {
		return errors.Wrapf(err, "Create Offer")
	}

	if err := pc.SetLocalDescription(offer); err != nil {
		return errors.Wrapf(err, "Set offer %v", offer)
	}

	answer, err := apiRtcRequest(ctx, "/rtc/v1/publish", r, offer.SDP)
	if err != nil {
		return errors.Wrapf(err, "Api request offer=%v", offer.SDP)
	}

	if err := pc.SetRemoteDescription(webrtc.SessionDescription{
		Type: webrtc.SDPTypeAnswer, SDP: answer,
	}); err != nil {
		return errors.Wrapf(err, "Set answer %v", answer)
	}

	ctx, cancel := context.WithCancel(ctx)

	// ICE state management.
	pc.OnICEConnectionStateChange(func(state webrtc.ICEConnectionState) {
		logger.Tf(ctx, "ICE state %v", state)
	})

	sAudioSender.Transport().OnStateChange(func(state webrtc.DTLSTransportState) {
		logger.Tf(ctx, "DTLS state %v", state)
	})

	pcDone, pcDoneCancel := context.WithCancel(context.Background())
	pc.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
		logger.Tf(ctx, "PC state %v", state)

		if state == webrtc.PeerConnectionStateConnected {
			pcDoneCancel()
		}

		if state == webrtc.PeerConnectionStateFailed || state == webrtc.PeerConnectionStateClosed {
			if ctx.Err() != nil {
				return
			}

			logger.Wf(ctx, "Close for PC state %v", state)
			cancel()
		}
	})

	// Wait for event from context or tracks.
	var wg sync.WaitGroup

	wg.Add(1)
	go func() {
		defer wg.Done()
		<-ctx.Done()

		if sAudioSender != nil {
			sAudioSender.Stop()
		}

		if sVideoSender != nil {
			sVideoSender.Stop()
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()

		if sAudioSender == nil {
			return
		}

		select {
		case <-ctx.Done():
		case <-pcDone.Done():
			logger.Tf(ctx, "PC(ICE+DTLS+SRTP) done, start read audio %v", sourceAudio)
		}

		buf := make([]byte, 1500)
		for ctx.Err() == nil {
			if _, _, err := sAudioSender.Read(buf); err != nil {
				return
			}
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()

		if sAudioTrack == nil {
			return
		}

		select {
		case <-ctx.Done():
		case <-pcDone.Done():
			logger.Tf(ctx, "PC(ICE+DTLS+SRTP) done, start ingest audio %v", sourceAudio)
		}

		for ctx.Err() == nil {
			if err := readAudioTrackFromDisk(ctx, sourceAudio, sAudioTrack); err != nil {
				if errors.Cause(err) == io.EOF {
					logger.Tf(ctx, "EOF, restart ingest audio %v", sourceAudio)
					continue
				}
				logger.Wf(ctx, "Ignore audio err %+v", err)
			}
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()

		if sVideoSender == nil {
			return
		}

		select {
		case <-ctx.Done():
		case <-pcDone.Done():
			logger.Tf(ctx, "PC(ICE+DTLS+SRTP) done, start read video %v", sourceVideo)
		}

		buf := make([]byte, 1500)
		for ctx.Err() == nil {
			if _, _, err := sVideoSender.Read(buf); err != nil {
				return
			}
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()

		if sVideoTrack == nil {
			return
		}

		select {
		case <-ctx.Done():
		case <-pcDone.Done():
			logger.Tf(ctx, "PC(ICE+DTLS+SRTP) done, start ingest video %v", sourceVideo)
		}

		for ctx.Err() == nil {
			if err := readVideoTrackFromDisk(ctx, sourceVideo, fps, sVideoTrack); err != nil {
				if errors.Cause(err) == io.EOF {
					logger.Tf(ctx, "EOF, restart ingest video %v", sourceVideo)
					continue
				}
				logger.Wf(ctx, "Ignore video err %+v", err)
			}
		}
	}()

	wg.Wait()
	return nil
}

func readAudioTrackFromDisk(ctx context.Context, source string, track *TrackLocalStaticSample) error {
	f, err := os.Open(source)
	if err != nil {
		return errors.Wrapf(err, "Open file %v", source)
	}
	defer f.Close()

	ogg, _, err := oggreader.NewWith(f)
	if err != nil {
		return errors.Wrapf(err, "Open ogg %v", source)
	}

	clock := NewWallClock()

	codec := track.Codec()
	var lastGranule uint64
	for ctx.Err() == nil {
		pageData, pageHeader, err := ogg.ParseNextPage()
		if err == io.EOF {
			return nil
		}
		if err != nil {
			return errors.Wrapf(err, "Read ogg")
		}

		// The amount of samples is the difference between the last and current timestamp
		sampleCount := uint64(pageHeader.GranulePosition - lastGranule)
		lastGranule = pageHeader.GranulePosition
		sampleDuration := time.Duration(uint64(time.Millisecond) * 1000 * sampleCount / uint64(codec.ClockRate))

		if err = track.WriteSample(media.Sample{Data: pageData, Duration: sampleDuration}); err != nil {
			return errors.Wrapf(err, "Write sample")
		}

		if d := clock.Tick(sampleDuration); d > 0 {
			time.Sleep(d)
		}
	}

	return nil
}

func readVideoTrackFromDisk(ctx context.Context, source string, fps int, track *TrackLocalStaticSample) error {
	f, err := os.Open(source)
	if err != nil {
		return errors.Wrapf(err, "Open file %v", source)
	}
	defer f.Close()

	// TODO: FIXME: Support ivf for vp8.
	h264, err := h264reader.NewReader(f)
	if err != nil {
		return errors.Wrapf(err, "Open h264 %v", source)
	}

	clock := NewWallClock()
	sampleDuration := time.Duration(uint64(time.Millisecond) * 1000 / uint64(fps))
	for ctx.Err() == nil {
		var sps, pps *h264reader.NAL
		var oFrames []*h264reader.NAL
		for ctx.Err() == nil {
			frame, err := h264.NextNAL()
			if err == io.EOF {
				return nil
			}
			if err != nil {
				return errors.Wrapf(err, "Read h264")
			}

			oFrames = append(oFrames, frame)
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

		var frames []*h264reader.NAL
		// Package SPS/PPS to STAP-A
		if sps != nil && pps != nil {
			stapA := packageAsSTAPA(sps, pps)
			frames = append(frames, stapA)
		}
		// Append other original frames.
		for _, frame := range oFrames {
			if frame.UnitType != h264reader.NalUnitTypeSPS && frame.UnitType != h264reader.NalUnitTypePPS {
				frames = append(frames, frame)
			}
		}

		// Covert frames to sample(buffers).
		for i, frame := range frames {
			sample := media.Sample{Data: frame.Data, Duration: sampleDuration}
			// Use the sample timestamp for frames.
			if i != len(frames)-1 {
				sample.Duration = 0
			}

			// For STAP-A, set marker to false, to make Chrome happy.
			track.OnBeforeWritePacket = func(p *rtp.Packet) {
				if i < len(frames)-1 {
					p.Header.Marker = false
				}
			}

			if err = track.WriteSample(sample); err != nil {
				return errors.Wrapf(err, "Write sample")
			}
		}

		if d := clock.Tick(sampleDuration); d > 0 {
			time.Sleep(d)
		}
	}

	return nil
}
