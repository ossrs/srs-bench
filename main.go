package main

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/pion/rtcp"
	"github.com/pion/webrtc/v3"
	"github.com/pion/webrtc/v3/pkg/media"
	"github.com/pion/webrtc/v3/pkg/media/h264writer"
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
	"time"
)

func main() {
	var pr, dump_audio, dump_video string
	var clients int
	flag.StringVar(&pr, "pr", "", "")
	flag.StringVar(&dump_audio, "da", "", "")
	flag.StringVar(&dump_video, "dv", "", "")
	flag.IntVar(&clients, "nn", 1, "")

	flag.Usage = func() {
		fmt.Println(fmt.Sprintf("Usage: %v [Options]", os.Args[0]))
		fmt.Println(fmt.Sprintf("Options:"))
		fmt.Println(fmt.Sprintf("   -nn     The number of clients to simulate. Default: 1"))
		fmt.Println(fmt.Sprintf("Player:"))
		fmt.Println(fmt.Sprintf("   -pr     The url to play."))
		fmt.Println(fmt.Sprintf("   -da     [Optional] The file path to dump audio, ignore if empty."))
		fmt.Println(fmt.Sprintf("   -dv     [Optional] The file path to dump video, ignore if empty."))
		fmt.Println(fmt.Sprintf("For example:"))
		fmt.Println(fmt.Sprintf("   %v -pr webrtc://localhost/live/livestream", os.Args[0]))
		fmt.Println(fmt.Sprintf("   %v -pr webrtc://localhost/live/livestream -da a.ogg -dv v.ivf", os.Args[0]))
	}
	flag.Parse()

	if pr == "" || clients <= 0 {
		flag.Usage()
		os.Exit(-1)
	}

	ctx, cancel := context.WithCancel(context.Background())
	logger.Tf(ctx, "The benchmark play(url=%v, da=%v, dv=%v), clients=%v",
		pr, dump_audio, dump_video, clients)

	if dump_video != "" && !strings.HasSuffix(dump_video, ".h264") && !strings.HasSuffix(dump_video, ".ivf") {
		logger.Ef(ctx, "Should be .ivf or .264, actual %v", dump_video)
		os.Exit(-1)
	}

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
			if err := startPlay(ctx, pr, da, dv); err != nil {
				logger.Wf(ctx, "Run err %+v", err)
			}
		}(da, dv)
	}

	wg.Wait()

	logger.Tf(ctx, "Done")
}

func startPlay(ctx context.Context, r, dump_audio, dump_video string) error {
	ctx = logger.WithContext(ctx)

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

	answer, err := apiRtcPlay(ctx, r, offer.SDP)
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
			if da == nil && dump_audio != "" {
				if da, err = oggwriter.New(dump_audio, codec.ClockRate, codec.Channels); err != nil {
					return errors.Wrapf(err, "New audio dumper")
				}
				logger.Tf(ctx, "Open ogg writer file=%v, tbn=%v, channels=%v",
					dump_audio, codec.ClockRate, codec.Channels)
			}

			if err = writeTrackToDisk(ctx, da, track); err != nil {
				return errors.Wrapf(err, "Write audio disk")
			}
		} else if codec.MimeType == "video/VP8" {
			if dump_video != "" && !strings.HasSuffix(dump_video, ".ivf") {
				return errors.Errorf("%v should be .ivf for VP8", dump_video)
			}

			if dv_vp8 == nil && dump_video != "" {
				if dv_vp8, err = ivfwriter.New(dump_video); err != nil {
					return errors.Wrapf(err, "New video dumper")
				}
				logger.Tf(ctx, "Open ivf writer file=%v", dump_video)
			}

			if err = writeTrackToDisk(ctx, dv_vp8, track); err != nil {
				return errors.Wrapf(err, "Write video disk")
			}
		} else if codec.MimeType == "video/H264" {
			if dump_video != "" && !strings.HasSuffix(dump_video, ".h264") {
				return errors.Errorf("%v should be .h264 for H264", dump_video)
			}

			if dv_h264 == nil && dump_video != "" {
				if dv_h264, err = h264writer.New(dump_video); err != nil {
					return errors.Wrapf(err, "New video dumper")
				}
				logger.Tf(ctx, "Open h264 writer file=%v", dump_video)
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
			logger.Wf(ctx, "Close for ICE state %v", state)
			cancel()
		}
	})

	<-ctx.Done()

	return err
}

func apiRtcPlay(ctx context.Context, r, offer string) (string, error) {
	u, err := url.Parse(r)
	if err != nil {
		return "", errors.Wrapf(err, "Parse url %v", r)
	}

	api := fmt.Sprintf("http://%v:1985/rtc/v1/play/", u.Hostname())

	reqBody := struct {
		Api       string `json:"api"`
		ClientIP  string `json:"clientip"`
		SDP       string `json:"sdp"`
		StreamURL string `json:"streamurl"`
	}{
		api, "", offer, r,
	}

	b, err := json.Marshal(reqBody)
	if err != nil {
		return "", errors.Wrapf(err, "Marshal body %v", reqBody)
	}
	logger.If(ctx, "Request url api=%v with %v", api, string(b))
	logger.Tf(ctx, "Request url api=%v with %v bytes", api, len(b))

	req, err := http.NewRequest("POST", api, strings.NewReader(string(b)))
	if err != nil {
		return "", errors.Wrapf(err, "HTTP request %v", string(b))
	}

	res, err := http.DefaultClient.Do(req)
	if err != nil {
		return "", errors.Wrapf(err, "Do HTTP request %v", string(b))
	}

	b2, err := ioutil.ReadAll(res.Body)
	if err != nil {
		return "", errors.Wrapf(err, "Read response for %v", string(b))
	}
	logger.If(ctx, "Response from %v is %v", api, string(b2))
	logger.Tf(ctx, "Response from %v is %v bytes", api, len(b2))

	resBody := struct {
		Code    int    `json:"code"`
		Session string `json:"sessionid"`
		SDP     string `json:"sdp"`
	}{}
	if err := json.Unmarshal(b2, &resBody); err != nil {
		return "", errors.Wrapf(err, "Marshal %v", string(b2))
	}

	if resBody.Code != 0 {
		return "", errors.Errorf("Server fail code=%v %v", resBody.Code, string(b2))
	}
	logger.If(ctx, "Parse response to code=%v, session=%v, sdp=%v",
		resBody.Code, resBody.Session, escapeSDP(resBody.SDP))
	logger.Tf(ctx, "Parse response to code=%v, session=%v, sdp=%v bytes",
		resBody.Code, resBody.Session, len(resBody.SDP))

	return string(resBody.SDP), nil
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

func escapeSDP(sdp string) string {
	return strings.ReplaceAll(strings.ReplaceAll(sdp, "\r", "\\r"), "\n", "\\n")
}
