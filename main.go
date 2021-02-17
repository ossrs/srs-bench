package main

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"github.com/ossrs/go-oryx-lib/errors"
	"github.com/ossrs/go-oryx-lib/logger"
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
	var sr, dumpAudio, dumpVideo string
	flag.StringVar(&sr, "sr", "", "")
	flag.StringVar(&dumpAudio, "da", "", "")
	flag.StringVar(&dumpVideo, "dv", "", "")

	var pr, sourceAudio, sourceVideo string
	var fps int
	flag.StringVar(&pr, "pr", "", "")
	flag.StringVar(&sourceAudio, "sa", "", "")
	flag.StringVar(&sourceVideo, "sv", "", "")
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
	if pr != "" && (sourceAudio == "" && sourceVideo == "") {
		showHelp = true
	}
	if showHelp {
		flag.Usage()
		os.Exit(-1)
	}

	ctx, cancel := context.WithCancel(context.Background())
	summaryDesc := fmt.Sprintf("clients=%v, delay=%v", clients, delay)
	if sr != "" {
		summaryDesc = fmt.Sprintf("%v, play(url=%v, da=%v, dv=%v)", summaryDesc, sr, dumpAudio, dumpVideo)
	}
	if pr != "" {
		summaryDesc = fmt.Sprintf("%v, publish(url=%v, sa=%v, sv=%v, fps=%v)", summaryDesc, pr, sourceAudio, sourceVideo, fps)
	}
	logger.Tf(ctx, "Start benchmark with %v", summaryDesc)

	checkFlag := func() error {
		if dumpVideo != "" && !strings.HasSuffix(dumpVideo, ".h264") && !strings.HasSuffix(dumpVideo, ".ivf") {
			return errors.Errorf("Should be .ivf or .264, actual %v", dumpVideo)
		}

		if sourceVideo != "" && !strings.HasSuffix(sourceVideo, ".h264") {
			return errors.Errorf("Should be .264, actual %v", sourceVideo)
		}

		if sourceVideo != "" && strings.HasSuffix(sourceVideo, ".h264") && fps <= 0 {
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

	for i := 0; sr != "" && i < streams && ctx.Err() == nil; i++ {
		r_auto := sr
		if streams > 1 && !strings.Contains(r_auto, "[s]") {
			r_auto += "_[s]"
		}
		r2 := strings.ReplaceAll(r_auto, "[s]", fmt.Sprintf("%v", i))

		for j := 0; sr != "" && j < clients && ctx.Err() == nil; j++ {
			// Dump audio or video only for the first client.
			da, dv := dumpAudio, dumpVideo
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

	for i := 0; pr != "" && i < streams && ctx.Err() == nil; i++ {
		r_auto := pr
		if streams > 1 && !strings.Contains(r_auto, "[s]") {
			r_auto += "_[s]"
		}
		r2 := strings.ReplaceAll(r_auto, "[s]", fmt.Sprintf("%v", i))

		wg.Add(1)
		go func(pr string) {
			defer wg.Done()
			if err := startPublish(ctx, pr, sourceAudio, sourceVideo, fps); err != nil {
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

	res, err := http.DefaultClient.Do(req.WithContext(ctx))
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

func escapeSDP(sdp string) string {
	return strings.ReplaceAll(strings.ReplaceAll(sdp, "\r", "\\r"), "\n", "\\n")
}
