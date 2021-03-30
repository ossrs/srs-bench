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
	"encoding/json"
	"fmt"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"io/ioutil"
	"net/http"
	"net/url"
	"strings"
	"sync"
	"time"
)

type janusReply struct {
	transactionID string
	replies       chan []byte
}

func newJanusReply(tid string) *janusReply {
	return &janusReply{
		transactionID: tid,
		replies:       make(chan []byte, 1),
	}
}

type janusAPI struct {
	r string
	// The ID created by API.
	sessionID   uint64
	handleID    uint64
	publisherID uint64
	// The callbacks.
	onWebrtcUp func(sender, sessionID uint64)
	onMedia    func(sender, sessionID uint64, mtype string, receiving bool)
	// The context for polling.
	pollingCtx    context.Context
	pollingCancel context.CancelFunc
	wg            sync.WaitGroup
	// The replies of polling key is transactionID, value is janusReply.
	replies sync.Map
}

func newJanusAPI(r string) *janusAPI {
	v := &janusAPI{r: r}
	v.onWebrtcUp = func(sender, sessionID uint64) {
	}
	v.onMedia = func(sender, sessionID uint64, mtype string, receiving bool) {
	}
	return v
}

func (v *janusAPI) Close() error {
	v.pollingCancel()
	v.wg.Wait()
	return nil
}

func (v *janusAPI) Create(ctx context.Context) error {
	if err := v.createSession(ctx); err != nil {
		return err
	}

	if err := v.attachPlugin(ctx); err != nil {
		return err
	}

	return nil
}

func (v *janusAPI) JoinAsPublisher(ctx context.Context, room int, display string) error {
	u, err := url.Parse(v.r)
	if err != nil {
		return errors.Wrapf(err, "Parse url %v", v.r)
	}

	api := fmt.Sprintf("http://%v/janus/%v/%v", u.Host, v.sessionID, v.handleID)

	reqBodyBody := struct {
		Request string `json:"request"`
		PType   string `json:"ptype"`
		Room    int    `json:"room"`
		Display string `json:"display"`
	}{
		"join", "publisher", room, display,
	}
	reqBody := struct {
		Janus       string      `json:"janus"`
		Transaction string      `json:"transaction"`
		Body        interface{} `json:"body"`
	}{
		"message", newTransactionID(), reqBodyBody,
	}

	reply := newJanusReply(reqBody.Transaction)
	v.replies.Store(reqBody.Transaction, reply)

	b, err := json.Marshal(reqBody)
	if err != nil {
		return errors.Wrapf(err, "Marshal body %v", reqBody)
	}
	logger.Tf(ctx, "Request url api=%v with %v", api, string(b))

	req, err := http.NewRequest("POST", api, strings.NewReader(string(b)))
	if err != nil {
		return errors.Wrapf(err, "HTTP request %v", string(b))
	}

	res, err := http.DefaultClient.Do(req.WithContext(ctx))
	if err != nil {
		return errors.Wrapf(err, "Do HTTP request %v", string(b))
	}

	b2, err := ioutil.ReadAll(res.Body)
	if err != nil {
		return errors.Wrapf(err, "Read response for %v", string(b))
	}

	s2 := escapeJSON(string(b2))
	logger.Tf(ctx, "Response from %v is %v", api, s2)

	ackBody := struct {
		Janus       string `json:"janus"`
		SessionID   uint64 `json:"session_id"`
		Transaction string `json:"transaction"`
	}{}
	if err := json.Unmarshal([]byte(s2), &ackBody); err != nil {
		return errors.Wrapf(err, "Marshal %v", s2)
	}
	if ackBody.Janus != "ack" {
		return errors.Errorf("Server fail code=%v %v", ackBody.Janus, s2)
	}
	logger.Tf(ctx, "Response tid=%v ack", reply.transactionID)

	// Reply from polling.
	var s3 string
	select {
	case <-ctx.Done():
		return nil
	case b3 := <-reply.replies:
		s3 = escapeJSON(string(b3))
		logger.Tf(ctx, "Async response tid=%v, reply=%v", reply.transactionID, s3)
	}
	resBody := struct {
		Janus       string `json:"janus"`
		Session     uint64 `json:"session_id"`
		Transaction string `json:"transaction"`
		Sender      uint64 `json:"sender"`
		PluginData  struct {
			Plugin string `json:"plugin"`
			Data   struct {
				VideoRoom   string        `json:"videoroom"`
				Room        int           `json:"room"`
				Description string        `json:"description"`
				ID          uint64        `json:"id"`
				PrivateID   uint64        `json:"private_id"`
				Publishers  []interface{} `json:"publishers"`
			} `json:"data"`
		} `json:"plugindata"`
	}{}
	if err := json.Unmarshal([]byte(s3), &resBody); err != nil {
		return errors.Wrapf(err, "Marshal %v", s3)
	}

	plugin := resBody.PluginData.Data
	if resBody.Janus != "event" || plugin.VideoRoom != "joined" {
		return errors.Errorf("Server fail janus=%v, plugin=%v %v", resBody.Janus, plugin.VideoRoom, s3)
	}

	v.publisherID = plugin.ID
	logger.Tf(ctx, "Join as publisher room=%v, display=%v, tid=%v ok, event=%v, plugin=%v, id=%v, publishers=%v",
		room, display, reply.transactionID, resBody.Janus, plugin.VideoRoom, v.publisherID, len(plugin.Publishers))

	return nil
}

func (v *janusAPI) Publish(ctx context.Context, offer string) (answer string, err error) {
	u, err := url.Parse(v.r)
	if err != nil {
		return "", errors.Wrapf(err, "Parse url %v", v.r)
	}

	api := fmt.Sprintf("http://%v/janus/%v/%v", u.Host, v.sessionID, v.handleID)

	reqBodyBody := struct {
		Request string `json:"request"`
		Video   bool   `json:"video"`
		Audio   bool   `json:"audio"`
	}{
		"configure", true, true,
	}
	jsepBody := struct {
		Type string `json:"type"`
		SDP  string `json:"sdp"`
	}{
		"offer", offer,
	}
	reqBody := struct {
		Janus       string      `json:"janus"`
		Transaction string      `json:"transaction"`
		Body        interface{} `json:"body"`
		JSEP        interface{} `json:"jsep"`
	}{
		"message", newTransactionID(), reqBodyBody, jsepBody,
	}

	reply := newJanusReply(reqBody.Transaction)
	v.replies.Store(reqBody.Transaction, reply)

	b, err := json.Marshal(reqBody)
	if err != nil {
		return "", errors.Wrapf(err, "Marshal body %v", reqBody)
	}
	logger.Tf(ctx, "Request url api=%v with %v", api, string(b))

	req, err := http.NewRequest("POST", api, strings.NewReader(string(b)))
	if err != nil {
		return "", errors.Wrapf(err, "HTTP request %v", string(b))
	}

	res, err := http.DefaultClient.Do(req.WithContext(ctx))
	if err != nil {
		return "", errors.Wrapf(err, "Do HTTP request %v", string(b))
	}

	b2, err := ioutil.ReadAll(res.Body)
	if err != nil {
		return "", errors.Wrapf(err, "Read response for %v", string(b))
	}

	s2 := escapeJSON(string(b2))
	logger.Tf(ctx, "Response from %v is %v", api, s2)

	ackBody := struct {
		Janus       string `json:"janus"`
		SessionID   uint64 `json:"session_id"`
		Transaction string `json:"transaction"`
	}{}
	if err := json.Unmarshal([]byte(s2), &ackBody); err != nil {
		return "", errors.Wrapf(err, "Marshal %v", s2)
	}
	if ackBody.Janus != "ack" {
		return "", errors.Errorf("Server fail code=%v %v", ackBody.Janus, s2)
	}
	logger.Tf(ctx, "Response tid=%v ack", reply.transactionID)

	// Reply from polling.
	var s3 string
	select {
	case <-ctx.Done():
		return "", nil
	case b3 := <-reply.replies:
		s3 = escapeJSON(string(b3))
		logger.Tf(ctx, "Async response tid=%v, reply=%v", reply.transactionID, s3)
	}
	resBody := struct {
		Janus       string `json:"janus"`
		Session     uint64 `json:"session_id"`
		Transaction string `json:"transaction"`
		Sender      uint64 `json:"sender"`
		PluginData  struct {
			Plugin string `json:"plugin"`
			Data   struct {
				VideoRoom  string `json:"videoroom"`
				Room       int    `json:"room"`
				Configured string `json:"configured"`
				AudioCodec string `json:"audio_codec"`
				VideoCodec string `json:"video_codec"`
			} `json:"data"`
		} `json:"plugindata"`
		JSEP struct {
			Type string `json:"type"`
			SDP  string `json:"sdp"`
		} `json:"jsep"`
	}{}
	if err := json.Unmarshal([]byte(s3), &resBody); err != nil {
		return "", errors.Wrapf(err, "Marshal %v", s3)
	}

	plugin := resBody.PluginData.Data
	jsep := resBody.JSEP
	if resBody.Janus != "event" || plugin.VideoRoom != "event" {
		return "", errors.Errorf("Server fail janus=%v, plugin=%v %v", resBody.Janus, plugin.VideoRoom, s3)
	}
	logger.Tf(ctx, "Configure publisher offer=%vB, tid=%v ok, event=%v, plugin=%v, answer=%vB",
		len(offer), reply.transactionID, resBody.Janus, plugin.VideoRoom, len(jsep.SDP))

	return jsep.SDP, nil
}

func (v *janusAPI) createSession(ctx context.Context) error {
	v.pollingCtx, v.pollingCancel = context.WithCancel(ctx)

	u, err := url.Parse(v.r)
	if err != nil {
		return errors.Wrapf(err, "Parse url %v", v.r)
	}

	api := fmt.Sprintf("http://%v/janus", u.Host)

	reqBody := struct {
		Janus       string `json:"janus"`
		Transaction string `json:"transaction"`
	}{
		"create", newTransactionID(),
	}

	b, err := json.Marshal(reqBody)
	if err != nil {
		return errors.Wrapf(err, "Marshal body %v", reqBody)
	}
	logger.Tf(ctx, "Request url api=%v with %v", api, string(b))

	req, err := http.NewRequest("POST", api, strings.NewReader(string(b)))
	if err != nil {
		return errors.Wrapf(err, "HTTP request %v", string(b))
	}

	res, err := http.DefaultClient.Do(req.WithContext(ctx))
	if err != nil {
		return errors.Wrapf(err, "Do HTTP request %v", string(b))
	}

	b2, err := ioutil.ReadAll(res.Body)
	if err != nil {
		return errors.Wrapf(err, "Read response for %v", string(b))
	}

	s2 := escapeJSON(string(b2))
	logger.Tf(ctx, "Response from %v is %v", api, s2)

	resBody := struct {
		Janus       string `json:"janus"`
		Transaction string `json:"transaction"`
		Data        struct {
			ID uint64 `json:"id"`
		} `json:"data"`
	}{}
	if err := json.Unmarshal([]byte(s2), &resBody); err != nil {
		return errors.Wrapf(err, "Marshal %v", s2)
	}
	if resBody.Janus != "success" {
		return errors.Errorf("Server fail code=%v %v", resBody.Janus, s2)
	}

	v.sessionID = resBody.Data.ID
	logger.Tf(ctx, "Parse create sessionID=%v", v.sessionID)

	v.wg.Add(1)
	go func() {
		defer v.wg.Done()
		defer v.pollingCancel()

		for v.pollingCtx.Err() == nil {
			if err := v.polling(v.pollingCtx); err != nil {
				if ctx.Err() != context.Canceled {
					logger.Wf(ctx, "polling err %+v", err)
				}
				break
			}
		}
	}()

	return nil
}

func (v *janusAPI) attachPlugin(ctx context.Context) error {
	u, err := url.Parse(v.r)
	if err != nil {
		return errors.Wrapf(err, "Parse url %v", v.r)
	}

	api := fmt.Sprintf("http://%v/janus/%v", u.Host, v.sessionID)

	reqBody := struct {
		Janus       string `json:"janus"`
		OpaqueID    string `json:"opaque_id"`
		Plugin      string `json:"plugin"`
		Transaction string `json:"transaction"`
	}{
		"attach", newTransactionID(),
		"janus.plugin.videoroom", newTransactionID(),
	}

	b, err := json.Marshal(reqBody)
	if err != nil {
		return errors.Wrapf(err, "Marshal body %v", reqBody)
	}
	logger.Tf(ctx, "Request url api=%v with %v", api, string(b))

	req, err := http.NewRequest("POST", api, strings.NewReader(string(b)))
	if err != nil {
		return errors.Wrapf(err, "HTTP request %v", string(b))
	}

	res, err := http.DefaultClient.Do(req.WithContext(ctx))
	if err != nil {
		return errors.Wrapf(err, "Do HTTP request %v", string(b))
	}

	b2, err := ioutil.ReadAll(res.Body)
	if err != nil {
		return errors.Wrapf(err, "Read response for %v", string(b))
	}

	s2 := escapeJSON(string(b2))
	logger.Tf(ctx, "Response from %v is %v", api, s2)

	resBody := struct {
		Janus       string `json:"janus"`
		SessionID   uint64 `json:"session_id"`
		Transaction string `json:"transaction"`
		Data        struct {
			ID uint64 `json:"id"`
		} `json:"data"`
	}{}
	if err := json.Unmarshal([]byte(s2), &resBody); err != nil {
		return errors.Wrapf(err, "Marshal %v", s2)
	}
	if resBody.Janus != "success" {
		return errors.Errorf("Server fail code=%v %v", resBody.Janus, s2)
	}

	v.handleID = resBody.Data.ID
	logger.Tf(ctx, "Parse create handleID=%v", v.handleID)

	return nil
}

func (v *janusAPI) polling(ctx context.Context) error {
	u, err := url.Parse(v.r)
	if err != nil {
		return errors.Wrapf(err, "Parse url %v", v.r)
	}

	api := fmt.Sprintf("http://%v/janus/%v?rid=%v&maxev=1", u.Host, v.sessionID,
		uint64(time.Duration(time.Now().UnixNano())/time.Millisecond))
	logger.Tf(ctx, "Polling: Request url api=%v", api)

	req, err := http.NewRequest("GET", api, nil)
	if err != nil {
		return errors.Wrapf(err, "HTTP request %v", api)
	}

	res, err := http.DefaultClient.Do(req.WithContext(ctx))
	if err != nil {
		return errors.Wrapf(err, "Do HTTP request %v", api)
	}

	b2, err := ioutil.ReadAll(res.Body)
	if err != nil {
		return errors.Wrapf(err, "Read response for %v", api)
	}

	s2 := escapeJSON(string(b2))
	logger.Tf(ctx, "Polling: Response from %v is %v", api, s2)

	if len(b2) == 0 {
		return nil
	}

	replyID := struct {
		Janus       string `json:"janus"`
		Transaction string `json:"transaction"`
	}{}
	if err := json.Unmarshal([]byte(s2), &replyID); err != nil {
		return errors.Wrapf(err, "Marshal %v", s2)
	}

	switch replyID.Janus {
	case "event":
		if r, ok := v.replies.Load(replyID.Transaction); !ok {
			logger.Wf(ctx, "Polling: Drop tid=%v reply %v", replyID.Transaction, s2)
		} else if r2, ok := r.(*janusReply); !ok {
			logger.Wf(ctx, "Polling: Ignore tid=%v reply %v", replyID.Transaction, s2)
		} else {
			select {
			case <-ctx.Done():
			case r2.replies <- b2:
				logger.Tf(ctx, "Polling: Reply tid=%v ok, %v", replyID.Transaction, s2)
			}
		}
	case "keepalive":
		return nil
	case "webrtcup", "media":
		if err := v.handleCall(replyID.Janus, s2); err != nil {
			logger.Wf(ctx, "Polling: Handle call %v fail %v, err %+v", replyID.Janus, s2, err)
		}
	default:
		logger.Wf(ctx, "Polling: Unknown janus=%v %v", replyID.Janus, s2)
	}

	return nil
}

func (v *janusAPI) handleCall(janus string, s string) error {
	type callHeader struct {
		Sender    uint64 `json:"sender"`
		SessionID uint64 `json:"session_id"`
	}

	switch janus {
	case "webrtcup":
		// {"janus":"webrtcup","sender":7698695982180732,"session_id":2403223275773854}
		r := callHeader{}
		if err := json.Unmarshal([]byte(s), &r); err != nil {
			return err
		}

		v.onWebrtcUp(r.Sender, r.SessionID)
	case "media":
		// {"janus":"media","receiving":true,"sender":7698695982180732,"session_id":2403223275773854,"type":"audio"}
		r := struct {
			callHeader
			Type      string `json:"type"`
			Receiving bool   `json:"receiving"`
		}{}
		if err := json.Unmarshal([]byte(s), &r); err != nil {
			return err
		}

		v.onMedia(r.Sender, r.SessionID, r.Type, r.Receiving)
	}

	return nil
}
