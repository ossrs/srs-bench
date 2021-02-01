package main

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/pion/webrtc/v3"
	"io/ioutil"
	"net/http"
	"net/url"
	"os"
	"strings"
)

func main() {
	var r string
	flag.StringVar(&r, "url", "", "")
	flag.Usage = func() {
		fmt.Println(fmt.Sprintf("Usage: %v [Options]", os.Args[0]))
		fmt.Println(fmt.Sprintf("Options:"))
		fmt.Println(fmt.Sprintf("   -url    The url to play"))
		fmt.Println(fmt.Sprintf("For example:"))
		fmt.Println(fmt.Sprintf("   %v -url webrtc://localhost/live/livestream", os.Args[0]))
	}
	flag.Parse()

	if r == "" {
		flag.Usage()
		os.Exit(-1)
	}

	ctx := context.Background()
	logger.Tf(ctx, "The benchmark url is %v", r)

	if err := run(ctx, r); err != nil {
		logger.Wf(ctx, "Run err %+v", err)
		os.Exit(-1)
	}

	logger.Tf(ctx, "Done")
}

func escapeSDP(sdp string) string {
	return strings.ReplaceAll(strings.ReplaceAll(sdp, "\r", "\\r"), "\n", "\\n")
}

func run(ctx context.Context, r string) error {
	ctx = logger.WithContext(ctx)

	u, err := url.Parse(r)
	if err != nil {
		return errors.Wrapf(err, "Parse url %v", r)
	}

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

	api := fmt.Sprintf("http://%v:1985/rtc/v1/play/", u.Hostname())

	reqBody := struct {
		Api       string `json:"api"`
		ClientIP  string `json:"clientip"`
		SDP       string `json:"sdp"`
		StreamURL string `json:"streamurl"`
	}{
		api, "", offer.SDP, r,
	}

	b, err := json.Marshal(reqBody)
	if err != nil {
		return errors.Wrapf(err, "Marshal body %v", reqBody)
	}
	logger.If(ctx, "Request url api=%v with %v", api, string(b))
	logger.Tf(ctx, "Request url api=%v with %v bytes", api, len(b))

	req, err := http.NewRequest("POST", api, strings.NewReader(string(b)))
	if err != nil {
		return errors.Wrapf(err, "HTTP request %v", string(b))
	}

	res, err := http.DefaultClient.Do(req)
	if err != nil {
		return errors.Wrapf(err, "Do HTTP request %v", string(b))
	}

	b2, err := ioutil.ReadAll(res.Body)
	if err != nil {
		return errors.Wrapf(err, "Read response for %v", string(b))
	}
	logger.If(ctx, "Response from %v is %v", api, string(b2))
	logger.Tf(ctx, "Response from %v is %v bytes", api, len(b2))

	resBody := struct {
		Code    int    `json:"code"`
		Session string `json:"sessionid"`
		SDP     string `json:"sdp"`
	}{}
	if err := json.Unmarshal(b2, &resBody); err != nil {
		return errors.Wrapf(err, "Marshal %v", string(b2))
	}

	if resBody.Code != 0 {
		return errors.Errorf("Server fail code=%v %v", resBody.Code, string(b2))
	}
	logger.If(ctx, "Parse response to code=%v, session=%v, sdp=%v",
		resBody.Code, resBody.Session, escapeSDP(resBody.SDP))
	logger.Tf(ctx, "Parse response to code=%v, session=%v, sdp=%v bytes",
		resBody.Code, resBody.Session, len(resBody.SDP))

	if err := pc.SetRemoteDescription(webrtc.SessionDescription{
		Type: webrtc.SDPTypeAnswer, SDP: string(resBody.SDP),
	}); err != nil {
		return errors.Wrapf(err, "Set answer %v", string(b2))
	}

	pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
		codec := track.Codec()
		logger.Tf(ctx, "Got track %v, pt=%v, tbn=%v %v",
			codec.MimeType, codec.PayloadType, codec.ClockRate, codec.SDPFmtpLine)
	})

	<-ctx.Done()

	return nil
}
