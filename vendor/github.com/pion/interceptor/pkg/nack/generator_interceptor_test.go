package nack

import (
	"testing"
	"time"

	"github.com/pion/interceptor"
	"github.com/pion/interceptor/internal/test"
	"github.com/pion/logging"
	"github.com/pion/rtcp"
	"github.com/pion/rtp"
	"github.com/stretchr/testify/assert"
)

func TestGeneratorInterceptor(t *testing.T) {
	const interval = time.Millisecond * 10
	i, err := NewGeneratorInterceptor(
		GeneratorSize(64),
		GeneratorSkipLastN(2),
		GeneratorInterval(interval),
		GeneratorLog(logging.NewDefaultLoggerFactory().NewLogger("test")),
	)
	assert.NoError(t, err)

	stream := test.NewMockStream(&interceptor.StreamInfo{
		SSRC:         1,
		RTCPFeedback: []interceptor.RTCPFeedback{{Type: "nack"}},
	}, i)
	defer func() {
		assert.NoError(t, stream.Close())
	}()

	for _, seqNum := range []uint16{10, 11, 12, 14, 16, 18} {
		stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{SequenceNumber: seqNum}})

		select {
		case r := <-stream.ReadRTP():
			assert.NoError(t, r.Err)
			assert.Equal(t, seqNum, r.Packet.SequenceNumber)
		case <-time.After(10 * time.Millisecond):
			t.Fatal("receiver rtp packet not found")
		}
	}

	time.Sleep(interval * 2) // wait for at least 2 nack packets

	select {
	case <-stream.WrittenRTCP():
		// ignore the first nack, it might only contain the sequence id 13 as missing
	default:
	}

	select {
	case pkts := <-stream.WrittenRTCP():
		assert.Equal(t, 1, len(pkts), "single packet RTCP Compound Packet expected")

		p, ok := pkts[0].(*rtcp.TransportLayerNack)
		assert.True(t, ok, "TransportLayerNack rtcp packet expected, found: %T", pkts[0])

		assert.Equal(t, uint16(13), p.Nacks[0].PacketID)
		assert.Equal(t, rtcp.PacketBitmap(0b10), p.Nacks[0].LostPackets) // we want packets: 13, 15 (not packet 17, because skipLastN is setReceived to 2)
	case <-time.After(10 * time.Millisecond):
		t.Fatal("written rtcp packet not found")
	}
}

func TestGeneratorInterceptor_InvalidSize(t *testing.T) {
	_, err := NewGeneratorInterceptor(GeneratorSize(5))
	assert.Error(t, err, ErrInvalidSize)
}
