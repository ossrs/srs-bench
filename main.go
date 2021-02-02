package main

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/pion/webrtc/v3"
	"github.com/pion/webrtc/v3/pkg/media"
	"github.com/pion/webrtc/v3/pkg/media/ivfwriter"
	"github.com/pion/webrtc/v3/pkg/media/oggwriter"
	"io/ioutil"
	"net/http"
	"net/url"
	"os"
	"os/signal"
	"strings"
	"sync"
	"syscall"
)

func main() {
	var r, dump_audio, dump_video string
	var clients int
	flag.StringVar(&r, "url", "", "")
	flag.StringVar(&dump_audio, "da", "", "")
	flag.StringVar(&dump_video, "dv", "", "")
	flag.IntVar(&clients, "nn", 1, "")

	flag.Usage = func() {
		fmt.Println(fmt.Sprintf("Usage: %v [Options]", os.Args[0]))
		fmt.Println(fmt.Sprintf("Options:"))
		fmt.Println(fmt.Sprintf("   -url    The url to play."))
		fmt.Println(fmt.Sprintf("   -da     [Optional] The file path to dump audio, ignore if empty."))
		fmt.Println(fmt.Sprintf("   -dv     [Optional] The file path to dump video, ignore if empty."))
		fmt.Println(fmt.Sprintf("   -nn     The number of clients to simulate. Default: 1"))
		fmt.Println(fmt.Sprintf("For example:"))
		fmt.Println(fmt.Sprintf("   %v -url webrtc://localhost/live/livestream", os.Args[0]))
		fmt.Println(fmt.Sprintf("   %v -url webrtc://localhost/live/livestream -da a.ogg -dv v.ivf", os.Args[0]))
	}
	flag.Parse()

	if r == "" || clients <= 0 {
		flag.Usage()
		os.Exit(-1)
	}

	ctx, cancel := context.WithCancel(context.Background())
	logger.Tf(ctx, "The benchmark url=%v, da=%v, dv=%v, clients=%v",
		r, dump_audio, dump_video, clients)

	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, syscall.SIGTERM, syscall.SIGINT)
	go func() {
		for sig := range sigs {
			logger.Wf(ctx, "Quit for signal %v", sig)
			cancel()
		}
	}()

	// Run tasks.
	var wg sync.WaitGroup

	for i := 0; i < clients; i++ {
		// Dump audio or video only for the first client.
		da, dv := dump_audio, dump_video
		if i > 0 {
			da, dv = "", ""
		}

		wg.Add(1)
		go func(da, dv string) {
			defer wg.Done()
			if err := run(ctx, r, da, dv); err != nil {
				logger.Wf(ctx, "Run err %+v", err)
			}
		}(da, dv)
	}

	wg.Wait()

	logger.Tf(ctx, "Done")
}

func escapeSDP(sdp string) string {
	return strings.ReplaceAll(strings.ReplaceAll(sdp, "\r", "\\r"), "\n", "\\n")
}

func run(ctx context.Context, r, dump_audio, dump_video string) error {
	ctx, cancel := context.WithCancel(logger.WithContext(ctx))

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

	var da media.Writer
	var dv media.Writer
	defer func() {
		if da != nil {
			da.Close()
		}
		if dv != nil {
			dv.Close()
		}
	}()

	pc.OnTrack(func(track *webrtc.TrackRemote, receiver *webrtc.RTPReceiver) {
		codec := track.Codec()
		if codec.MimeType == "audio/opus" {
			logger.Tf(ctx, "Got track %v, pt=%v, tbn=%v %v",
				codec.MimeType, codec.PayloadType, codec.ClockRate, codec.Channels)

			if da == nil && dump_audio != "" {
				if da, err = oggwriter.New(dump_audio, codec.ClockRate, codec.Channels); err != nil {
					cancel()
				}
				logger.Tf(ctx, "Open ogg writer file=%v, tbn=%v, channels=%v",
					dump_audio, codec.ClockRate, codec.Channels)
			}

			if err = writeDisk(ctx, da, track); err != nil {
				cancel()
			}
		} else if codec.MimeType == "video/VP8" {
			logger.Tf(ctx, "Got track %v, pt=%v, tbn=%v %v",
				codec.MimeType, codec.PayloadType, codec.ClockRate, codec.SDPFmtpLine)

			if dv == nil && dump_video != "" {
				if dv, err = ivfwriter.New(dump_video); err != nil {
					cancel()
				}
				logger.Tf(ctx, "Open ivf writer file=%v", dump_video)
			}

			if err = writeDisk(ctx, dv, track); err != nil {
				cancel()
			}
		}
	})

	<-ctx.Done()

	return nil
}

func writeDisk(ctx context.Context, w media.Writer, track *webrtc.TrackRemote) error {
	for ctx.Err() != nil {
		pkt, _, err := track.ReadRTP()
		if err != nil {
			return err
		}

		if w == nil {
			continue
		}

		if err := w.WriteRTP(pkt); err != nil {
			return err
		}
	}

	return ctx.Err()
}
