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
	"fmt"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/pion/interceptor"
	"github.com/pion/sdp/v3"
	"github.com/pion/webrtc/v3"
	"net/url"
	"strconv"
	"strings"
	"time"
)

func startPlay(ctx context.Context, r string, enableAudioLevel, enableTWCC bool, pli int) error {
	ctx = logger.WithContext(ctx)

	u, err := url.Parse(r)
	if err != nil {
		return errors.Wrapf(err, "Parse url %v", r)
	}

	var room int
	var display string
	if us := strings.SplitN(u.Path, "/", 3); len(us) >= 3 {
		if iv, err := strconv.Atoi(us[1]); err != nil {
			return errors.Wrapf(err, "parse %v", us[1])
		} else {
			room = iv
		}

		display = strings.Join(us[2:], "-")
	}

	logger.Tf(ctx, "Run play url=%v, room=%v, diplay=%v, audio-level=%v, twcc=%v",
		r, room, display, enableAudioLevel, enableTWCC)

	// For audio-level.
	webrtcNewPeerConnection := func(configuration webrtc.Configuration) (*webrtc.PeerConnection, error) {
		m := &webrtc.MediaEngine{}
		if err := m.RegisterDefaultCodecs(); err != nil {
			return nil, err
		}

		for _, extension := range []string{sdp.SDESMidURI, sdp.SDESRTPStreamIDURI, sdp.TransportCCURI} {
			if extension == sdp.TransportCCURI && !enableTWCC {
				continue
			}
			if err := m.RegisterHeaderExtension(webrtc.RTPHeaderExtensionCapability{URI: extension}, webrtc.RTPCodecTypeVideo); err != nil {
				return nil, err
			}
		}

		// https://github.com/pion/ion/issues/130
		// https://github.com/pion/ion-sfu/pull/373/files#diff-6f42c5ac6f8192dd03e5a17e9d109e90cb76b1a4a7973be6ce44a89ffd1b5d18R73
		for _, extension := range []string{sdp.SDESMidURI, sdp.SDESRTPStreamIDURI, sdp.AudioLevelURI} {
			if extension == sdp.AudioLevelURI && !enableAudioLevel {
				continue
			}
			if err := m.RegisterHeaderExtension(webrtc.RTPHeaderExtensionCapability{URI: extension}, webrtc.RTPCodecTypeAudio); err != nil {
				return nil, err
			}
		}

		i := &interceptor.Registry{}
		if err := webrtc.RegisterDefaultInterceptors(m, i); err != nil {
			return nil, err
		}

		api := webrtc.NewAPI(webrtc.WithMediaEngine(m), webrtc.WithInterceptorRegistry(i))
		return api.NewPeerConnection(configuration)
	}

	pc, err := webrtcNewPeerConnection(webrtc.Configuration{})
	if err != nil {
		return errors.Wrapf(err, "Create PC")
	}

	var receivers []*webrtc.RTPReceiver
	defer func() {
		pc.Close()
		for _, receiver := range receivers {
			receiver.Stop()
		}
	}()

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

	// Signaling API
	api := newJanusAPI(fmt.Sprintf("http://%v/janus", u.Host))

	if err := api.Create(ctx); err != nil {
		return errors.Wrapf(err, "create")
	}
	defer api.Close()

	// Discover the publisher to subscribe.
	publisher, err := api.DiscoverPublisher(ctx, room, display, 5*time.Second)
	if err != nil {
		return err
	}
	if publisher == nil {
		return nil // Cancelled, ignore.
	}
	logger.Tf(ctx, "Publisher found, room=%v, display=%v, %v", room, display, publisher)

	<-ctx.Done()
	return nil
}
