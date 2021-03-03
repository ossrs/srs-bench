package srs_test

import (
	"flag"
	"net"
	"testing"
)

var rtcServer = flag.String("rtc-server", "127.0.0.1", "The RTC server to connect to")

func TestRTCServer(t *testing.T) {
	if ip := net.ParseIP(*rtcServer); ip == nil {
		t.Errorf("Invalid rtc server %v %v", *rtcServer, ip)
	}
}
