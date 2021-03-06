package srs

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/pion/interceptor"
	"github.com/pion/rtcp"
	"github.com/pion/rtp"
	"github.com/pion/webrtc/v3"
	"io/ioutil"
	"net/http"
	"os"
	"path"
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

func prepareTest() error {
	var err error

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

	// Check file.
	tryOpenFile := func(filename string) (string, error) {
		if filename == "" {
			return filename, nil
		}

		f, err := os.Open(filename)
		if err != nil {
			nfilename := path.Join("../", filename)
			if fi, r0 := os.Stat(nfilename); r0 != nil && !fi.IsDir() {
				return filename, errors.Wrapf(err, "No video file at %v or %v", filename, nfilename)
			}

			return nfilename, nil
		}
		defer f.Close()

		return filename, nil
	}

	if *srsPublishVideo, err = tryOpenFile(*srsPublishVideo); err != nil {
		return err
	}

	if *srsPublishAudio, err = tryOpenFile(*srsPublishAudio); err != nil {
		return err
	}

	// Disable the logger during all tests.
	if *srsLog == false {
		olw := logger.Switch(ioutil.Discard)
		defer func() {
			logger.Switch(olw)
		}()
	}

	return nil
}

func TestMain(m *testing.M) {
	if err := prepareTest(); err != nil {
		logger.Ef(nil, "Prepare test fail, err %+v", err)
		os.Exit(-1)
	}

	os.Exit(m.Run())
}

type TestPlayer struct {
	onPacket  func(p *rtp.Packet)
	pc        *webrtc.PeerConnection
	receivers []*webrtc.RTPReceiver
}

func (v *TestPlayer) Close() error {
	if v.pc != nil {
		v.pc.Close()
		v.pc = nil
	}

	for _, receiver := range v.receivers {
		receiver.Stop()
	}
	v.receivers = nil

	return nil
}

func (v *TestPlayer) Run(ctx context.Context, cancel context.CancelFunc) error {
	r := fmt.Sprintf("%v://%v%v", srsSchema, *srsServer, *srsStream)
	pli := time.Duration(*srsPlayPLI) * time.Millisecond
	logger.Tf(ctx, "Start play url=%v", r)

	pc, err := webrtc.NewPeerConnection(webrtc.Configuration{})
	if err != nil {
		return errors.Wrapf(err, "Create PC")
	}
	v.pc = pc

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

		v.receivers = append(v.receivers, receiver)

		pkt, _, err := track.ReadRTP()
		if err != nil {
			return errors.Wrapf(err, "Read RTP")
		}

		if v.onPacket != nil {
			v.onPacket(pkt)
		}

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
	onOffer  func(s *webrtc.SessionDescription) error
	onPacket func(p *rtp.Header, payload []byte)
	ready    context.CancelFunc
	// internal objects
	aIngester *audioIngester
	vIngester *videoIngester
	pc        *webrtc.PeerConnection
}

func (v *TestPublisher) Close() error {
	if v.vIngester != nil {
		v.vIngester.Close()
	}

	if v.aIngester != nil {
		v.aIngester.Close()
	}

	if v.pc != nil {
		v.pc.Close()
	}
	return nil
}

func (v *TestPublisher) Run(ctx context.Context, cancel context.CancelFunc) error {
	r := fmt.Sprintf("%v://%v%v", srsSchema, *srsServer, *srsStream)
	sourceVideo := *srsPublishVideo
	sourceAudio := *srsPublishAudio
	fps := *srsPublishVideoFps

	logger.Tf(ctx, "Start publish url=%v, audio=%v, video=%v, fps=%v",
		r, sourceAudio, sourceVideo, fps)

	// Filter for SPS/PPS marker.
	onPacketInterceptor := &rtpInterceptor{}
	onPacketInterceptor.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
		if v.onPacket != nil {
			v.onPacket(header, payload)
		}
		return onPacketInterceptor.nextRTPWriter.Write(header, payload, attributes)
	}

	webrtcNewPeerConnection := func(configuration webrtc.Configuration) (*webrtc.PeerConnection, error) {
		m := &webrtc.MediaEngine{}
		if err := m.RegisterDefaultCodecs(); err != nil {
			return nil, err
		}

		registry := &interceptor.Registry{}
		if err := webrtc.RegisterDefaultInterceptors(m, registry); err != nil {
			return nil, err
		}

		for _, bi := range []interceptor.Interceptor{onPacketInterceptor} {
			registry.Add(bi)
		}

		if sourceAudio != "" {
			v.aIngester = NewAudioIngester(sourceAudio)
			registry.Add(v.aIngester.audioLevelInterceptor)
		}
		if sourceVideo != "" {
			v.vIngester = NewVideoIngester(sourceVideo)
			registry.Add(v.vIngester.markerInterceptor)
		}

		api := webrtc.NewAPI(webrtc.WithMediaEngine(m), webrtc.WithInterceptorRegistry(registry))
		return api.NewPeerConnection(configuration)
	}

	pc, err := webrtcNewPeerConnection(webrtc.Configuration{})
	if err != nil {
		return errors.Wrapf(err, "Create PC")
	}
	v.pc = pc

	if v.vIngester != nil {
		if err := v.vIngester.AddTrack(pc, fps); err != nil {
			return errors.Wrapf(err, "Add track")
		}
		defer v.vIngester.Close()
	}

	if v.aIngester != nil {
		if err := v.aIngester.AddTrack(pc); err != nil {
			return errors.Wrapf(err, "Add track")
		}
		defer v.aIngester.Close()
	}

	offer, err := pc.CreateOffer(nil)
	if err != nil {
		return errors.Wrapf(err, "Create Offer")
	}

	if err := pc.SetLocalDescription(offer); err != nil {
		return errors.Wrapf(err, "Set offer %v", offer)
	}

	if v.onOffer != nil {
		if err := v.onOffer(&offer); err != nil {
			return errors.Wrapf(err, "sdp %v %v", offer.Type, offer.SDP)
		}
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
	var finalErr error

	wg.Add(1)
	go func() {
		defer wg.Done()

		if v.aIngester == nil {
			return
		}

		select {
		case <-ctx.Done():
		case <-pcDone.Done():
		}

		wg.Add(1)
		go func() {
			defer wg.Done()

			buf := make([]byte, 1500)
			for ctx.Err() == nil {
				if _, _, err := v.aIngester.sAudioSender.Read(buf); err != nil {
					return
				}
			}
		}()

		if err := v.aIngester.Ingest(ctx); err != nil {
			if err != context.Canceled {
				finalErr = errors.Wrapf(err, "audio")
			}
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()

		if v.vIngester == nil {
			return
		}

		select {
		case <-ctx.Done():
		case <-pcDone.Done():
			logger.Tf(ctx, "PC(ICE+DTLS+SRTP) done, start ingest video %v", sourceVideo)
		}

		wg.Add(1)
		go func() {
			defer wg.Done()

			buf := make([]byte, 1500)
			for ctx.Err() == nil {
				if _, _, err := v.vIngester.sVideoSender.Read(buf); err != nil {
					return
				}
			}
		}()

		if err := v.vIngester.Ingest(ctx); err != nil {
			if err != context.Canceled {
				finalErr = errors.Wrapf(err, "video")
			}
		}
	}()

	wg.Wait()
	return finalErr
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
	playOK := *srsPlayOKPackets

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

		var nn uint64
		p := &TestPlayer{onPacket: func(p *rtp.Packet) {
			nn++
			if nn >= uint64(playOK) {
				cancel() // Completed.
			}
		}}
		defer p.Close()

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
		defer p.Close()

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

func TestRTCServerDTLSArq(t *testing.T) {
	ctx := logger.WithContext(context.Background())
	ctx, cancel := context.WithTimeout(ctx, time.Duration(*srsTimeout)*time.Millisecond)
	publishOK := *srsPublishOKPackets

	var ss []*webrtc.RTPReceiver
	for _, s := range ss {
		s.Stop()
	}

	var nn int64
	p := &TestPublisher{onPacket: func(p *rtp.Header, payload []byte) {
		if nn++; nn >= int64(publishOK) {
			cancel() // Send enough packets, done.
		}
	}, onOffer: testUtilSetupActive}
	defer p.Close()

	if err := p.Run(ctx, cancel); err != nil {
		if err != context.Canceled {
			t.Errorf("Error ctx=%v err %+v", ctx.Err(), err)
		}
	}
}
