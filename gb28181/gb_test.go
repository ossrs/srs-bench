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
	"github.com/ghettovoice/gosip/sip"
	"github.com/ossrs/go-oryx-lib/logger"
	"testing"
	"time"
)

func TestGbPublishRegularly(t *testing.T) {
	ctx := logger.WithContext(context.Background())
	ctx, cancel := context.WithTimeout(ctx, time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	err := func() error {
		t := NewGBTestPublisher()
		defer t.Close()

		var nnPackets int
		t.ingester.onSendPacket = func(pack *PSPackStream) error {
			if nnPackets += 1; nnPackets > 10 {
				cancel()
			}
			return nil
		}

		if err := t.Run(ctx); err != nil {
			return err
		}

		return nil
	}()
	if err := filterTestError(ctx.Err(), err); err != nil {
		t.Errorf("err %+v", err)
	}
}

func TestGbSessionHandshake(t *testing.T) {
	ctx := logger.WithContext(context.Background())
	ctx, cancel := context.WithTimeout(ctx, time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	err := func() error {
		t := NewGBTestSession()
		defer t.Close()
		if err := t.Run(ctx); err != nil {
			return err
		}

		var nn int
		t.session.onMessageHeartbeat = func(req, res sip.Message) error {
			if nn++; nn > 3 {
				t.session.cancel()
			}
			return nil
		}

		<- t.session.heartbeatCtx.Done()
		return t.session.heartbeatCtx.Err()
	}()
	if err := filterTestError(ctx.Err(), err); err != nil {
		t.Errorf("err %+v", err)
	}
}

func TestGbSessionHandshakeDropRegisterOk(t *testing.T) {
	ctx := logger.WithContext(context.Background())
	ctx, cancel := context.WithTimeout(ctx, time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	var conf *SIPConfig
	r0 := func() error {
		t := NewGBTestSession()
		defer t.Close()

		conf = t.session.sip.conf

		ctx, cancel2 := context.WithCancel(ctx)
		t.session.onRegisterDone = func(req, res sip.Message) error {
			cancel2()
			return nil
		}

		return t.Run(ctx)
	}()

	// Use the same session for SIP.
	r1 := func() error {
		session := NewGBTestSession()
		session.session.sip.conf = conf
		defer session.Close()
		return session.Run(ctx)
	}()
	if err := filterTestError(ctx.Err(), r0, r1); err != nil {
		t.Errorf("err %+v", err)
	}
}

func TestGbSessionHandshakeDropInviteRequest(t *testing.T) {
	ctx := logger.WithContext(context.Background())
	ctx, cancel := context.WithTimeout(ctx, time.Duration(*srsTimeout)*time.Millisecond)
	defer cancel()

	var conf *SIPConfig
	r0 := func() error {
		t := NewGBTestSession()
		defer t.Close()

		conf = t.session.sip.conf

		ctx, cancel2 := context.WithCancel(ctx)
		t.session.onInviteRequest = func(req sip.Message) error {
			cancel2()
			return nil
		}

		return t.Run(ctx)
	}()

	// Use the same session for SIP.
	r1 := func() error {
		t := NewGBTestSession()
		t.session.sip.conf = conf
		defer t.Close()
		return t.Run(ctx)
	}()
	if err := filterTestError(ctx.Err(), r0, r1); err != nil {
		t.Errorf("err %+v", err)
	}
}
