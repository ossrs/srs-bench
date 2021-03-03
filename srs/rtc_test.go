package srs

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/pion/rtcp"
	"github.com/pion/webrtc/v3"
	"io/ioutil"
	"net/http"
	"os"
	"testing"
	"time"
)

var srsServer = flag.String("srs-server", "127.0.0.1", "The RTC server to connect to")
var srsTimeout = flag.Int("srs-timeout", 3000, "For each case, the timeout in ms")
var srsPlayOKPackets = flag.Int("srs-play-ok-packets", 10, "If got N packets, it's ok, or fail")

func TestMain(m *testing.M) {
	// Disable the logger during all tests.
	olw := logger.Switch(ioutil.Discard)
	defer func() {
		logger.Switch(olw)
	}()

	// Run tests.
	os.Exit(m.Run())
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

func TestRTCServerPlay(t *testing.T) {
	ctx := context.Background()

	pc, err := webrtc.NewPeerConnection(webrtc.Configuration{})
	if err != nil {
		t.Error(err, "Create PC")
		return
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
		t.Error(err, "Create Offer")
		return
	}

	if err := pc.SetLocalDescription(offer); err != nil {
		t.Error("Set offer", offer, err)
		return
	}

	r := fmt.Sprintf("http://%v/live/livestream", *srsServer)
	answer, err := apiRtcRequest(ctx, "/rtc/v1/play", r, offer.SDP)
	if err != nil {
		t.Error("Api request offer", offer.SDP, err)
		return
	}

	if err := pc.SetRemoteDescription(webrtc.SessionDescription{
		Type: webrtc.SDPTypeAnswer, SDP: answer,
	}); err != nil {
		t.Error("Set answer", answer, err)
		return
	}

	ctx, cancel := context.WithCancel(ctx)
	pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
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

		// Try to read packets of track.
		for i := 0; i < *srsPlayOKPackets && ctx.Err() == nil; i++ {
			_, _, err := track.ReadRTP()
			if err != nil && ctx.Err() == nil {
				t.Error("Read packet", err)
			}
		}

		// Got enough packets, done.
		cancel()
	})

	pc.OnICEConnectionStateChange(func(state webrtc.ICEConnectionState) {
		if state == webrtc.ICEConnectionStateFailed || state == webrtc.ICEConnectionStateClosed {
			if ctx.Err() == nil {
				t.Errorf("Close for ICE state %v", state)
				cancel()
			}
		}
	})

	select {
	case <-ctx.Done():
	case <-time.After(time.Duration(*srsTimeout) * time.Millisecond):
		t.Error("Timeout")
		cancel()
	}
}
