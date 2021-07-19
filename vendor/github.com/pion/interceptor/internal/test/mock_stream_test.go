package test

import (
	"testing"
	"time"

	"github.com/pion/interceptor"
	"github.com/pion/rtcp"
	"github.com/pion/rtp"
	"github.com/stretchr/testify/assert"
)

func TestMockStream(t *testing.T) {
	s := NewMockStream(&interceptor.StreamInfo{}, &interceptor.NoOp{})

	assert.NoError(t, s.WriteRTCP([]rtcp.Packet{&rtcp.PictureLossIndication{}}))

	select {
	case <-s.WrittenRTCP():
	case <-time.After(10 * time.Millisecond):
		t.Error("rtcp packet written but not found")
	}
	select {
	case <-s.WrittenRTCP():
		t.Error("single rtcp packet written, but multiple found")
	case <-time.After(10 * time.Millisecond):
	}

	assert.NoError(t, s.WriteRTP(&rtp.Packet{}))

	select {
	case <-s.WrittenRTP():
	case <-time.After(10 * time.Millisecond):
		t.Error("rtp packet written but not found")
	}
	select {
	case <-s.WrittenRTP():
		t.Error("single rtp packet written, but multiple found")
	case <-time.After(10 * time.Millisecond):
	}

	s.ReceiveRTCP([]rtcp.Packet{&rtcp.PictureLossIndication{}})
	select {
	case r := <-s.ReadRTCP():
		if r.Err != nil {
			t.Errorf("read rtcp returned error: %v", r.Err)
		}
	case <-time.After(10 * time.Millisecond):
		t.Error("rtcp packet received but not read")
	}
	select {
	case r := <-s.ReadRTCP():
		t.Errorf("single rtcp packet received, but multiple read: %v", r)
	case <-time.After(10 * time.Millisecond):
	}

	s.ReceiveRTP(&rtp.Packet{})
	select {
	case r := <-s.ReadRTP():
		if r.Err != nil {
			t.Errorf("read rtcp returned error: %v", r.Err)
		}
	case <-time.After(10 * time.Millisecond):
		t.Error("rtp packet received but not read")
	}
	select {
	case r := <-s.ReadRTP():
		t.Errorf("single rtp packet received, but multiple read: %v", r)
	case <-time.After(10 * time.Millisecond):
	}

	assert.NoError(t, s.Close())
}
