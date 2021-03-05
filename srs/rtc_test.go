package srs

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/ossrs/srs-bench/rtc"
	"github.com/pion/rtcp"
	"github.com/pion/rtp"
	"github.com/pion/webrtc/v3"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"strings"
	"sync"
	"testing"
	"time"
)

var srsSchema = "http"
var srsHttps = flag.Bool("srs-https", false, "Whther connect to HTTPS-API")
var srsServer = flag.String("srs-server", "127.0.0.1", "The RTC server to connect to")
var srsStream = flag.String("srs-stream", "/rtc/regression", "The RTC stream to play")
var srsLog = flag.Bool("srs-log", false, "Whether enable the detail log")
var srsTimeout = flag.Int("srs-timeout", 3000, "For each case, the timeout in ms")
var srsPlayPLI = flag.Int("srs-play-pli", 5000, "The PLI interval in seconds for player.")
var srsPlayOKPackets = flag.Int("srs-play-ok-packets", 10, "If got N packets, it's ok, or fail")
var srsPublishOKPackets = flag.Int("srs-publish-ok-packets", 10, "If send N packets, it's ok, or fail")
var srsPublishAudio = flag.String("srs-publish-audio", "avatar.ogg", "The audio file for publisher.")
var srsPublishVideo = flag.String("srs-publish-video", "avatar.h264", "The video file for publisher.")
var srsPublishVideoFps = flag.Int("srs-publish-video-fps", 25, "The video fps for publisher.")

func TestMain(m *testing.M) {
	// Should parse it first.
	flag.Parse()

	// The stream should starts with /, for example, /rtc/regression
	if strings.HasPrefix(*srsStream, "/") {
		*srsStream = "/" + *srsStream
	}

	// Generate srs protocol from whether use HTTPS.
	if *srsHttps {
		srsSchema = "https"
	}

	// Disable the logger during all tests.
	if *srsLog == false {
		olw := logger.Switch(ioutil.Discard)
		defer func() {
			logger.Switch(olw)
		}()
	}

	// Run tests.
	os.Exit(m.Run())
}

type TestPlayer struct {
}

func (v *TestPlayer) Run(ctx context.Context, cancel context.CancelFunc) error {
	r := fmt.Sprintf("%v://%v%v", srsSchema, *srsServer, *srsStream)
	pli := time.Duration(*srsPlayPLI) * time.Millisecond
	playOK := *srsPlayOKPackets
	logger.Tf(ctx, "Start play url=%v", r)

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

	handleTrack := func(ctx context.Context, track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) error {
		// Send a PLI on an interval so that the publisher is pushing a keyframe
		go func() {
			if track.Kind() == webrtc.RTPCodecTypeAudio {
				return
			}

			for {
				select {
				case <-ctx.Done():
					return
				case <-time.After(pli):
					_ = pc.WriteRTCP([]rtcp.Packet{&rtcp.PictureLossIndication{
						MediaSSRC: uint32(track.SSRC()),
					}})
				}
			}
		}()

		// Try to read packets of track.
		for i := 0; i < playOK && ctx.Err() == nil; i++ {
			_, _, err := track.ReadRTP()
			if err != nil {
				return errors.Wrapf(err, "Read RTP")
			}
		}

		// Completed.
		cancel()

		return nil
	}

	pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
		err = handleTrack(ctx, track, receiver)
		if err != nil {
			codec := track.Codec()
			err = errors.Wrapf(err, "Handle  track %v, pt=%v", codec.MimeType, codec.PayloadType)
			cancel()
		}
	})

	pc.OnICEConnectionStateChange(func(state webrtc.ICEConnectionState) {
		if state == webrtc.ICEConnectionStateFailed || state == webrtc.ICEConnectionStateClosed {
			err = errors.Errorf("Close for ICE state %v", state)
			cancel()
		}
	})

	<-ctx.Done()
	return err
}

type TestPublisher struct {
	onPacket func(p *rtp.Packet)
	ready    context.CancelFunc
}

func (v *TestPublisher) Run(ctx context.Context, cancel context.CancelFunc) error {
	r := fmt.Sprintf("%v://%v%v", srsSchema, *srsServer, *srsStream)
	sourceVideo := *srsPublishVideo
	sourceAudio := *srsPublishAudio
	fps := *srsPublishVideoFps

	logger.Tf(ctx, "Start publish url=%v, audio=%v, video=%v, fps=%v",
		r, sourceAudio, sourceVideo, fps)

	pc, err := webrtc.NewPeerConnection(webrtc.Configuration{})
	if err != nil {
		return errors.Wrapf(err, "Create PC")
	}
	defer pc.Close()

	var sVideoTrack *rtc.TrackLocalStaticSample
	var sVideoSender *webrtc.RTPSender
	if sourceVideo != "" {
		mimeType, trackID := "video/H264", "video"
		if strings.HasSuffix(sourceVideo, ".ivf") {
			mimeType = "video/VP8"
		}

		sVideoTrack, err = rtc.NewTrackLocalStaticSample(
			webrtc.RTPCodecCapability{MimeType: mimeType, ClockRate: 90000}, trackID, "pion",
		)
		if err != nil {
			return errors.Wrapf(err, "Create video track")
		}

		sVideoSender, err = pc.AddTrack(sVideoTrack)
		if err != nil {
			return errors.Wrapf(err, "Add video track")
		}
		sVideoSender.Stop()
	}

	var sAudioTrack *rtc.TrackLocalStaticSample
	var sAudioSender *webrtc.RTPSender
	if sourceAudio != "" {
		mimeType, trackID := "audio/opus", "audio"
		sAudioTrack, err = rtc.NewTrackLocalStaticSample(
			webrtc.RTPCodecCapability{MimeType: mimeType, ClockRate: 48000, Channels: 2}, trackID, "pion",
		)
		if err != nil {
			return errors.Wrapf(err, "Create audio track")
		}

		sAudioSender, err = pc.AddTrack(sAudioTrack)
		if err != nil {
			return errors.Wrapf(err, "Add audio track")
		}
		defer sAudioSender.Stop()
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

	logger.Tf(ctx, "State signaling=%v, ice=%v, conn=%v", pc.SignalingState(), pc.ICEConnectionState(), pc.ConnectionState())

	pcDone, pcDoneCancel := context.WithCancel(context.Background())
	pc.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
		logger.Tf(ctx, "PC state %v", state)

		if state == webrtc.PeerConnectionStateConnected {
			pcDoneCancel()
			if v.ready != nil {
				v.ready()
			}
		}

		if state == webrtc.PeerConnectionStateFailed || state == webrtc.PeerConnectionStateClosed {
			err = errors.Errorf("Close for PC state %v", state)
			cancel()
		}
	})

	// Wait for event from context or tracks.
	var wg sync.WaitGroup

	wg.Add(1)
	go func() {
		defer wg.Done()

		if sAudioSender == nil {
			return
		}

		select {
		case <-ctx.Done():
		case <-pcDone.Done():
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
		}

		for ctx.Err() == nil {
			if err := readAudioTrackFromDisk(ctx, sourceAudio, sAudioSender, sAudioTrack, v.onPacket); err != nil {
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
			logger.Tf(ctx, "PC(ICE+DTLS+SRTP) done, start read video packets")
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
			if err := readVideoTrackFromDisk(ctx, sourceVideo, sVideoSender, fps, sVideoTrack); err != nil {
				if errors.Cause(err) == io.EOF {
					logger.Tf(ctx, "EOF, restart ingest video %v", sourceVideo)
					continue
				}
				logger.Wf(ctx, "Ignore video err %+v", err)
			}
		}
	}()

	wg.Wait()
	return err
}

func TestRTCServerVersion(t *testing.T) {
	api := fmt.Sprintf("http://%v:1985/api/v1/versions", *srsServer)
	req, err := http.NewRequest("POST", api, nil)
	if err != nil {
		t.Errorf("Request %v", api)
		return
	}

	res, err := http.DefaultClient.Do(req)
	if err != nil {
		t.Errorf("Do request %v", api)
		return
	}

	b, err := ioutil.ReadAll(res.Body)
	if err != nil {
		t.Errorf("Read body of %v", api)
		return
	}

	obj := struct {
		Code   int    `json:"code"`
		Server string `json:"server"`
		Data   struct {
			Major    int    `json:"major"`
			Minor    int    `json:"minor"`
			Revision int    `json:"revision"`
			Version  string `json:"version"`
		} `json:"data"`
	}{}
	if err := json.Unmarshal(b, &obj); err != nil {
		t.Errorf("Parse %v", string(b))
		return
	}
	if obj.Code != 0 {
		t.Errorf("Server err code=%v, server=%v", obj.Code, obj.Server)
		return
	}
	if obj.Data.Major == 0 && obj.Data.Minor == 0 {
		t.Errorf("Invalid version %v", obj.Data)
		return
	}
}

func TestRTCServerPublishPlay(t *testing.T) {
	ctx := logger.WithContext(context.Background())
	ctx, cancel := context.WithTimeout(ctx, time.Duration(*srsTimeout)*time.Millisecond)

	publishReady, publishReadyCancel := context.WithCancel(context.Background())

	var wg sync.WaitGroup
	var r0, r1 error

	wg.Add(1)
	go func() {
		defer wg.Done()
		defer cancel()

		// Wait for publisher to start first.
		select {
		case <-ctx.Done():
			return
		case <-publishReady.Done():
		}

		p := &TestPlayer{}
		if err := p.Run(logger.WithContext(ctx), cancel); err != nil {
			if errors.Cause(err) != context.Canceled {
				r0 = err
			}
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		defer cancel()

		p := &TestPublisher{ready: publishReadyCancel}
		if err := p.Run(logger.WithContext(ctx), cancel); err != nil {
			if errors.Cause(err) != context.Canceled {
				r0 = err
			}
		}
	}()

	// Handle errs, the test result.
	wg.Wait()

	if r0 != nil || r1 != nil {
		t.Errorf("Error ctx %v r0 %+v, r1 %+v", ctx.Err(), r0, r1)
	}
}

func TestRTCServerPublishDTLS(t *testing.T) {
	ctx := logger.WithContext(context.Background())
	ctx, cancel := context.WithTimeout(ctx, time.Duration(*srsTimeout)*time.Millisecond)
	publishOK := *srsPublishOKPackets

	var nn int64
	onSendPacket := func(p *rtp.Packet) {
		nn++

		// Got enough packets, done.
		if nn >= int64(publishOK) {
			cancel()
		}
	}

	p := &TestPublisher{onPacket: onSendPacket}
	if err := p.Run(ctx, cancel); err != nil {
		if err != context.Canceled {
			t.Errorf("Error ctx=%v err %+v", ctx.Err(), err)
		}
	}
}
