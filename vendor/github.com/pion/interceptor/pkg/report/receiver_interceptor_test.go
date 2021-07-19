package report

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

func TestReceiverInterceptor(t *testing.T) {
	t.Run("before any packet", func(t *testing.T) {
		mt := test.MockTime{}
		i, err := NewReceiverInterceptor(
			ReceiverInterval(time.Millisecond*50),
			ReceiverLog(logging.NewDefaultLoggerFactory().NewLogger("test")),
			ReceiverNow(mt.Now),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{
			SSRC:      123456,
			ClockRate: 90000,
		}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, len(pkts), 1)
		rr, ok := pkts[0].(*rtcp.ReceiverReport)
		assert.True(t, ok)
		assert.Equal(t, 1, len(rr.Reports))
		assert.Equal(t, rtcp.ReceptionReport{
			SSRC:               uint32(123456),
			LastSequenceNumber: 0,
			LastSenderReport:   0,
			FractionLost:       0,
			TotalLost:          0,
			Delay:              0,
			Jitter:             0,
		}, rr.Reports[0])
	})

	rtpTime := time.Date(2009, time.November, 10, 23, 0, 0, 0, time.UTC)

	t.Run("after RTP packets", func(t *testing.T) {
		mt := test.MockTime{}
		i, err := NewReceiverInterceptor(
			ReceiverInterval(time.Millisecond*50),
			ReceiverLog(logging.NewDefaultLoggerFactory().NewLogger("test")),
			ReceiverNow(mt.Now),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{
			SSRC:      123456,
			ClockRate: 90000,
		}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		for i := 0; i < 10; i++ {
			stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{
				SequenceNumber: uint16(i),
			}})
		}

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, len(pkts), 1)
		rr, ok := pkts[0].(*rtcp.ReceiverReport)
		assert.True(t, ok)
		assert.Equal(t, 1, len(rr.Reports))
		assert.Equal(t, rtcp.ReceptionReport{
			SSRC:               uint32(123456),
			LastSequenceNumber: 9,
			LastSenderReport:   0,
			FractionLost:       0,
			TotalLost:          0,
			Delay:              0,
			Jitter:             0,
		}, rr.Reports[0])
	})

	t.Run("after RTP and RTCP packets", func(t *testing.T) {
		mt := test.MockTime{}
		i, err := NewReceiverInterceptor(
			ReceiverInterval(time.Millisecond*50),
			ReceiverLog(logging.NewDefaultLoggerFactory().NewLogger("test")),
			ReceiverNow(mt.Now),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{
			SSRC:      123456,
			ClockRate: 90000,
		}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		for i := 0; i < 10; i++ {
			stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{
				SequenceNumber: uint16(i),
			}})
		}

		now := time.Date(2009, time.November, 10, 23, 0, 1, 0, time.UTC)
		stream.ReceiveRTCP([]rtcp.Packet{
			&rtcp.SenderReport{
				SSRC:        123456,
				NTPTime:     ntpTime(now),
				RTPTime:     987654321 + uint32(now.Sub(rtpTime).Seconds()*90000),
				PacketCount: 10,
				OctetCount:  0,
			},
		})

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, len(pkts), 1)
		rr, ok := pkts[0].(*rtcp.ReceiverReport)
		assert.True(t, ok)
		assert.Equal(t, 1, len(rr.Reports))
		assert.Equal(t, rtcp.ReceptionReport{
			SSRC:               uint32(123456),
			LastSequenceNumber: 9,
			LastSenderReport:   1861287936,
			FractionLost:       0,
			TotalLost:          0,
			Delay:              rr.Reports[0].Delay,
			Jitter:             0,
		}, rr.Reports[0])
	})

	t.Run("overflow", func(t *testing.T) {
		mt := test.MockTime{}
		i, err := NewReceiverInterceptor(
			ReceiverInterval(time.Millisecond*50),
			ReceiverLog(logging.NewDefaultLoggerFactory().NewLogger("test")),
			ReceiverNow(mt.Now),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{
			SSRC:      123456,
			ClockRate: 90000,
		}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{
			SequenceNumber: 0xffff,
		}})

		stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{
			SequenceNumber: 0x00,
		}})

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, len(pkts), 1)
		rr, ok := pkts[0].(*rtcp.ReceiverReport)
		assert.True(t, ok)
		assert.Equal(t, 1, len(rr.Reports))
		assert.Equal(t, rtcp.ReceptionReport{
			SSRC:               uint32(123456),
			LastSequenceNumber: 1<<16 | 0x0000,
			LastSenderReport:   0,
			FractionLost:       0,
			TotalLost:          0,
			Delay:              0,
			Jitter:             0,
		}, rr.Reports[0])
	})

	t.Run("packet loss", func(t *testing.T) {
		mt := test.MockTime{}
		i, err := NewReceiverInterceptor(
			ReceiverInterval(time.Millisecond*50),
			ReceiverLog(logging.NewDefaultLoggerFactory().NewLogger("test")),
			ReceiverNow(mt.Now),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{
			SSRC:      123456,
			ClockRate: 90000,
		}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{
			SequenceNumber: 0x01,
		}})

		stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{
			SequenceNumber: 0x03,
		}})

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, len(pkts), 1)
		rr, ok := pkts[0].(*rtcp.ReceiverReport)
		assert.True(t, ok)
		assert.Equal(t, 1, len(rr.Reports))
		assert.Equal(t, rtcp.ReceptionReport{
			SSRC:               uint32(123456),
			LastSequenceNumber: 0x03,
			LastSenderReport:   0,
			FractionLost:       256 * 1 / 3,
			TotalLost:          1,
			Delay:              0,
			Jitter:             0,
		}, rr.Reports[0])

		now := time.Date(2009, time.November, 10, 23, 0, 1, 0, time.UTC)
		stream.ReceiveRTCP([]rtcp.Packet{
			&rtcp.SenderReport{
				SSRC:        123456,
				NTPTime:     ntpTime(now),
				RTPTime:     987654321 + uint32(now.Sub(rtpTime).Seconds()*90000),
				PacketCount: 10,
				OctetCount:  0,
			},
		})

		pkts = <-stream.WrittenRTCP()
		assert.Equal(t, len(pkts), 1)
		rr, ok = pkts[0].(*rtcp.ReceiverReport)
		assert.True(t, ok)
		assert.Equal(t, 1, len(rr.Reports))
		assert.Equal(t, rtcp.ReceptionReport{
			SSRC:               uint32(123456),
			LastSequenceNumber: 0x03,
			LastSenderReport:   1861287936,
			FractionLost:       0,
			TotalLost:          1,
			Delay:              rr.Reports[0].Delay,
			Jitter:             0,
		}, rr.Reports[0])
	})

	t.Run("overflow and packet loss", func(t *testing.T) {
		mt := test.MockTime{}
		i, err := NewReceiverInterceptor(
			ReceiverInterval(time.Millisecond*50),
			ReceiverLog(logging.NewDefaultLoggerFactory().NewLogger("test")),
			ReceiverNow(mt.Now),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{
			SSRC:      123456,
			ClockRate: 90000,
		}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{
			SequenceNumber: 0xffff,
		}})

		stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{
			SequenceNumber: 0x01,
		}})

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, len(pkts), 1)
		rr, ok := pkts[0].(*rtcp.ReceiverReport)
		assert.True(t, ok)
		assert.Equal(t, 1, len(rr.Reports))
		assert.Equal(t, rtcp.ReceptionReport{
			SSRC:               uint32(123456),
			LastSequenceNumber: 1<<16 | 0x01,
			LastSenderReport:   0,
			FractionLost:       256 * 1 / 3,
			TotalLost:          1,
			Delay:              0,
			Jitter:             0,
		}, rr.Reports[0])
	})

	t.Run("reordered packets", func(t *testing.T) {
		mt := test.MockTime{}
		i, err := NewReceiverInterceptor(
			ReceiverInterval(time.Millisecond*50),
			ReceiverLog(logging.NewDefaultLoggerFactory().NewLogger("test")),
			ReceiverNow(mt.Now),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{
			SSRC:      123456,
			ClockRate: 90000,
		}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		for _, seqNum := range []uint16{0x01, 0x03, 0x02, 0x04} {
			stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{
				SequenceNumber: seqNum,
			}})
		}

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, len(pkts), 1)
		rr, ok := pkts[0].(*rtcp.ReceiverReport)
		assert.True(t, ok)
		assert.Equal(t, 1, len(rr.Reports))
		assert.Equal(t, rtcp.ReceptionReport{
			SSRC:               uint32(123456),
			LastSequenceNumber: 0x04,
			LastSenderReport:   0,
			FractionLost:       0,
			TotalLost:          0,
			Delay:              0,
			Jitter:             0,
		}, rr.Reports[0])
	})

	t.Run("jitter", func(t *testing.T) {
		mt := test.MockTime{}
		i, err := NewReceiverInterceptor(
			ReceiverInterval(time.Millisecond*50),
			ReceiverLog(logging.NewDefaultLoggerFactory().NewLogger("test")),
			ReceiverNow(mt.Now),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{
			SSRC:      123456,
			ClockRate: 90000,
		}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		mt.SetNow(time.Date(2009, time.November, 10, 23, 0, 0, 0, time.UTC))
		stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{
			SequenceNumber: 0x01,
			Timestamp:      42378934,
		}})
		<-stream.ReadRTP()

		mt.SetNow(time.Date(2009, time.November, 10, 23, 0, 1, 0, time.UTC))
		stream.ReceiveRTP(&rtp.Packet{Header: rtp.Header{
			SequenceNumber: 0x02,
			Timestamp:      42378934 + 60000,
		}})

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, len(pkts), 1)
		rr, ok := pkts[0].(*rtcp.ReceiverReport)
		assert.True(t, ok)
		assert.Equal(t, 1, len(rr.Reports))
		assert.Equal(t, rtcp.ReceptionReport{
			SSRC:               uint32(123456),
			LastSequenceNumber: 0x02,
			LastSenderReport:   0,
			FractionLost:       0,
			TotalLost:          0,
			Delay:              0,
			Jitter:             30000 / 16,
		}, rr.Reports[0])
	})

	t.Run("delay", func(t *testing.T) {
		mt := test.MockTime{}
		i, err := NewReceiverInterceptor(
			ReceiverInterval(time.Millisecond*50),
			ReceiverLog(logging.NewDefaultLoggerFactory().NewLogger("test")),
			ReceiverNow(mt.Now),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{
			SSRC:      123456,
			ClockRate: 90000,
		}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		mt.SetNow(time.Date(2009, time.November, 10, 23, 0, 0, 0, time.UTC))
		stream.ReceiveRTCP([]rtcp.Packet{
			&rtcp.SenderReport{
				SSRC:        123456,
				NTPTime:     ntpTime(time.Date(2009, time.November, 10, 23, 0, 0, 0, time.UTC)),
				RTPTime:     987654321,
				PacketCount: 0,
				OctetCount:  0,
			},
		})
		<-stream.ReadRTCP()

		mt.SetNow(time.Date(2009, time.November, 10, 23, 0, 1, 0, time.UTC))
		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, len(pkts), 1)
		rr, ok := pkts[0].(*rtcp.ReceiverReport)
		assert.True(t, ok)
		assert.Equal(t, 1, len(rr.Reports))
		assert.Equal(t, rtcp.ReceptionReport{
			SSRC:               uint32(123456),
			LastSequenceNumber: 0,
			LastSenderReport:   1861222400,
			FractionLost:       0,
			TotalLost:          0,
			Delay:              65536,
			Jitter:             0,
		}, rr.Reports[0])
	})
}
