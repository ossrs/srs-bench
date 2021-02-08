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
	"github.com/pion/webrtc/v3/pkg/media/h264reader"
	"github.com/pion/webrtc/v3/pkg/media/h264writer"
	"github.com/pion/webrtc/v3/pkg/media/ivfwriter"
	"github.com/pion/webrtc/v3/pkg/media/oggreader"
	"github.com/pion/webrtc/v3/pkg/media/oggwriter"
	"io"
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
	var sr, dump_audio, dump_video string
	flag.StringVar(&sr, "sr", "", "")
	flag.StringVar(&dump_audio, "da", "", "")
	flag.StringVar(&dump_video, "dv", "", "")

	var pr, source_audio, source_video string
	var fps int
	flag.StringVar(&pr, "pr", "", "")
	flag.StringVar(&source_audio, "sa", "", "")
	flag.StringVar(&source_video, "sv", "", "")
	flag.IntVar(&fps, "fps", 0, "")

	var clients, streams, delay int
	flag.IntVar(&clients, "nn", 1, "")
	flag.IntVar(&streams, "sn", 1, "")
	flag.IntVar(&delay, "delay", 50, "")

	flag.Usage = func() {
		fmt.Println(fmt.Sprintf("Usage: %v [Options]", os.Args[0]))
		fmt.Println(fmt.Sprintf("Options:"))
		fmt.Println(fmt.Sprintf("   -nn     The number of clients to simulate. Default: 1"))
		fmt.Println(fmt.Sprintf("   -sn     The number of streams to simulate. Variable: [s]. Default: 1"))
		fmt.Println(fmt.Sprintf("   -delay  The start delay in ms for each client or stream to simulate. Default: 50"))
		fmt.Println(fmt.Sprintf("Player or Subscriber:"))
		fmt.Println(fmt.Sprintf("   -sr     The url to play/subscribe. If sn exceed 1, auto append variable [s]."))
		fmt.Println(fmt.Sprintf("   -da     [Optional] The file path to dump audio, ignore if empty."))
		fmt.Println(fmt.Sprintf("   -dv     [Optional] The file path to dump video, ignore if empty."))
		fmt.Println(fmt.Sprintf("Publisher:"))
		fmt.Println(fmt.Sprintf("   -pr     The url to publish. If sn exceed 1, auto append variable [s]."))
		fmt.Println(fmt.Sprintf("   -fps    The fps of .h264 source file."))
		fmt.Println(fmt.Sprintf("   -sa     [Optional] The file path to read audio, ignore if empty."))
		fmt.Println(fmt.Sprintf("   -sv     [Optional] The file path to read video, ignore if empty."))
		fmt.Println(fmt.Sprintf("\n例如，1个播放，1个推流:"))
		fmt.Println(fmt.Sprintf("   %v -sr webrtc://localhost/live/livestream", os.Args[0]))
		fmt.Println(fmt.Sprintf("   %v -pr webrtc://localhost/live/livestream -sa a.ogg -sv v.h264 -fps 25", os.Args[0]))
		fmt.Println(fmt.Sprintf("\n例如，1个流，3个播放，共3个客户端："))
		fmt.Println(fmt.Sprintf("   %v -sr webrtc://localhost/live/livestream -nn 3", os.Args[0]))
		fmt.Println(fmt.Sprintf("   %v -pr webrtc://localhost/live/livestream -sa a.ogg -sv v.h264 -fps 25", os.Args[0]))
		fmt.Println(fmt.Sprintf("\n例如，2个流，每个流3个播放，共6个客户端："))
		fmt.Println(fmt.Sprintf("   %v -sr webrtc://localhost/live/livestream_[s] -sn 2 -nn 3", os.Args[0]))
		fmt.Println(fmt.Sprintf("   %v -pr webrtc://localhost/live/livestream_[s] -sn 2 -sa a.ogg -sv v.h264 -fps 25", os.Args[0]))
		fmt.Println(fmt.Sprintf("\n例如，2个推流："))
		fmt.Println(fmt.Sprintf("   %v -pr webrtc://localhost/live/livestream_[s] -sn 2 -sa a.ogg -sv v.h264 -fps 25", os.Args[0]))
		fmt.Println(fmt.Sprintf("\n例如，1个录制："))
		fmt.Println(fmt.Sprintf("   %v -sr webrtc://localhost/live/livestream -da a.ogg -dv v.h264", os.Args[0]))
		fmt.Println()
	}
	flag.Parse()

	showHelp := (clients <= 0 || streams <= 0)
	if sr == "" && pr == "" {
		showHelp = true
	}
	if pr != "" && (source_audio == "" && source_video == "") {
		showHelp = true
	}
	if showHelp {
		flag.Usage()
		os.Exit(-1)
	}

	ctx, cancel := context.WithCancel(context.Background())
	summaryDesc := fmt.Sprintf("clients=%v, delay=%v", clients, delay)
	if sr != "" {
		summaryDesc = fmt.Sprintf("%v, play(url=%v, da=%v, dv=%v)", summaryDesc, sr, dump_audio, dump_video)
	}
	if pr != "" {
		summaryDesc = fmt.Sprintf("%v, publish(url=%v, sa=%v, sv=%v, fps=%v)", summaryDesc, pr, source_audio, source_video, fps)
	}
	logger.Tf(ctx, "Start benchmark with %v", summaryDesc)

	checkFlag := func() error {
		if dump_video != "" && !strings.HasSuffix(dump_video, ".h264") && !strings.HasSuffix(dump_video, ".ivf") {
			return errors.Errorf("Should be .ivf or .264, actual %v", dump_video)
		}

		if source_video != "" && !strings.HasSuffix(source_video, ".h264") {
			return errors.Errorf("Should be .264, actual %v", source_video)
		}

		if source_video != "" && strings.HasSuffix(source_video, ".h264") && fps <= 0 {
			return errors.Errorf("Video fps should >0, actual %v", fps)
		}
		return nil
	}
	if err := checkFlag(); err != nil {
		logger.Ef(ctx, "Check faile err %+v", err)
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

	for i := 0; sr != "" && i < streams; i++ {
		r_auto := sr
		if streams > 1 && !strings.Contains(r_auto, "[s]") {
			r_auto += "_[s]"
		}
		r2 := strings.ReplaceAll(r_auto, "[s]", fmt.Sprintf("%v", i))

		for j := 0; sr != "" && j < clients; j++ {
			// Dump audio or video only for the first client.
			da, dv := dump_audio, dump_video
			if i > 0 {
				da, dv = "", ""
			}

			wg.Add(1)
			go func(sr, da, dv string) {
				defer wg.Done()
				if err := startPlay(ctx, sr, da, dv); err != nil {
					if errors.Cause(err) != context.Canceled {
						logger.Wf(ctx, "Run err %+v", err)
					}
				}
			}(r2, da, dv)

			time.Sleep(time.Duration(delay) * time.Millisecond)
		}
	}

	for i := 0; pr != "" && i < streams; i++ {
		r_auto := pr
		if streams > 1 && !strings.Contains(r_auto, "[s]") {
			r_auto += "_[s]"
		}
		r2 := strings.ReplaceAll(r_auto, "[s]", fmt.Sprintf("%v", i))

		wg.Add(1)
		go func(pr string) {
			defer wg.Done()
			if err := startPublish(ctx, pr, source_audio, source_video, fps); err != nil {
				if errors.Cause(err) != context.Canceled {
					logger.Wf(ctx, "Run err %+v", err)
				}
			}
		}(r2)

		time.Sleep(time.Duration(delay) * time.Millisecond)
	}

	wg.Wait()

	logger.Tf(ctx, "Done")
}

// @see https://github.com/pion/webrtc/blob/master/examples/save-to-disk/main.go
func startPlay(ctx context.Context, r, dump_audio, dump_video string) error {
	ctx = logger.WithContext(ctx)

	logger.Tf(ctx, "Start play url=%v, audio=%v, video=%v", r, dump_audio, dump_video)

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
			if ctx.Err() != nil {
				return
			}

			logger.Wf(ctx, "Close for ICE state %v", state)
			cancel()
		}
	})

	<-ctx.Done()
	return err
}

func apiRtcRequest(ctx context.Context, apiPath, r, offer string) (string, error) {
	u, err := url.Parse(r)
	if err != nil {
		return "", errors.Wrapf(err, "Parse url %v", r)
	}

	// Build api url.
	host := u.Host
	if !strings.Contains(host, ":") {
		host += ":1985"
	}

	api := fmt.Sprintf("http://%v", host)
	if !strings.HasPrefix(apiPath, "/") {
		api += "/"
	}
	api += apiPath

	if !strings.HasSuffix(apiPath, "/") {
		api += "/"
	}

	// Build JSON body.
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

// @see https://github.com/pion/webrtc/blob/master/examples/play-from-disk/main.go
func startPublish(ctx context.Context, r, source_audio, source_video string, fps int) error {
	ctx = logger.WithContext(ctx)

	logger.Tf(ctx, "Start publish url=%v, audio=%v, video=%v, fps=%v", r, source_audio, source_video, fps)

	pc, err := webrtc.NewPeerConnection(webrtc.Configuration{})
	if err != nil {
		return errors.Wrapf(err, "Create PC")
	}

	var sv_track *webrtc.TrackLocalStaticSample
	var sv_sender *webrtc.RTPSender
	if source_video != "" {
		mimeType, trackID := "video/H264", "video"
		if strings.HasSuffix(source_video, ".ivf") {
			mimeType = "video/VP8"
		}

		sv_track, err = webrtc.NewTrackLocalStaticSample(
			webrtc.RTPCodecCapability{MimeType: mimeType, ClockRate: 90000}, trackID, "pion",
		)
		if err != nil {
			return errors.Wrapf(err, "Create video track")
		}

		sv_sender, err = pc.AddTrack(sv_track)
		if err != nil {
			return errors.Wrapf(err, "Add video track")
		}
	}

	var sa_track *webrtc.TrackLocalStaticSample
	var sa_sender *webrtc.RTPSender
	if source_audio != "" {
		mimeType, trackID := "audio/opus", "audio"
		sa_track, err = webrtc.NewTrackLocalStaticSample(
			webrtc.RTPCodecCapability{MimeType: mimeType, ClockRate: 48000, Channels: 2}, trackID, "pion",
		)
		if err != nil {
			return errors.Wrapf(err, "Create audio track")
		}

		sa_sender, err = pc.AddTrack(sa_track)
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
	iceDone := make(chan bool, 1)
	pc.OnICEConnectionStateChange(func(state webrtc.ICEConnectionState) {
		logger.If(ctx, "ICE state %v", state)

		if state == webrtc.ICEConnectionStateConnected {
			close(iceDone)
		}

		if state == webrtc.ICEConnectionStateFailed || state == webrtc.ICEConnectionStateClosed {
			if ctx.Err() != nil {
				return
			}

			logger.Wf(ctx, "Close for ICE state %v", state)
			cancel()
		}
	})

	// Wait for event from context or tracks.
	var wg sync.WaitGroup

	wg.Add(1)
	go func() {
		defer wg.Done()
		<-ctx.Done()

		if sa_sender != nil {
			sa_sender.Stop()
		}

		if sv_sender != nil {
			sv_sender.Stop()
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()

		if sa_sender == nil {
			return
		}

		buf := make([]byte, 1500)
		for ctx.Err() == nil {
			if _, _, err := sa_sender.Read(buf); err != nil {
				return
			}
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()

		if sa_track == nil {
			return
		}

		select {
		case <-ctx.Done():
		case <-iceDone:
			logger.Tf(ctx, "ICE done, start ingest audio %v", source_audio)
		}

		for ctx.Err() == nil {
			if err := readAudioTrackFromDisk(ctx, source_audio, sa_track); err != nil {
				if errors.Cause(err) == io.EOF {
					logger.Tf(ctx, "EOF, restart ingest audio %v", source_audio)
					continue
				}
				logger.Wf(ctx, "Ignore audio err %+v", err)
			}
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()

		if sv_sender == nil {
			return
		}

		buf := make([]byte, 1500)
		for ctx.Err() == nil {
			if _, _, err := sv_sender.Read(buf); err != nil {
				return
			}
		}
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()

		if sv_track == nil {
			return
		}

		select {
		case <-ctx.Done():
		case <-iceDone:
			logger.Tf(ctx, "ICE done, start ingest video %v", source_video)
		}

		for ctx.Err() == nil {
			if err := readVideoTrackFromDisk(ctx, source_video, fps, sv_track); err != nil {
				if errors.Cause(err) == io.EOF {
					logger.Tf(ctx, "EOF, restart ingest video %v", source_video)
					continue
				}
				logger.Wf(ctx, "Ignore video err %+v", err)
			}
		}
	}()

	wg.Wait()
	return nil
}

func readAudioTrackFromDisk(ctx context.Context, source string, track *webrtc.TrackLocalStaticSample) error {
	f, err := os.Open(source)
	if err != nil {
		return errors.Wrapf(err, "Open file %v", source)
	}
	defer f.Close()

	ogg, _, err := oggreader.NewWith(f)
	if err != nil {
		return errors.Wrapf(err, "Open ogg %v", source)
	}

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

		time.Sleep(sampleDuration)
	}

	return nil
}

func readVideoTrackFromDisk(ctx context.Context, source string, fps int, track *webrtc.TrackLocalStaticSample) error {
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

	sleepTime := time.Duration(uint64(time.Millisecond) * 1000 / uint64(fps))
	for ctx.Err() == nil {
		frame, err := h264.NextNAL()
		if err == io.EOF {
			return nil
		}
		if err != nil {
			return errors.Wrapf(err, "Read h264")
		}

		if err = track.WriteSample(media.Sample{Data: frame.Data, Duration: sleepTime}); err != nil {
			return errors.Wrapf(err, "Write sample")
		}

		time.Sleep(sleepTime)
	}

	return nil
}
