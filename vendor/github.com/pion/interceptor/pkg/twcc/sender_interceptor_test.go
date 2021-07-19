package twcc

import (
	"testing"
	"time"

	"github.com/pion/interceptor"
	"github.com/pion/interceptor/internal/test"
	"github.com/pion/rtcp"
	"github.com/pion/rtp"
	"github.com/stretchr/testify/assert"
)

func TestSenderInterceptor(t *testing.T) {
	t.Run("before any packets", func(t *testing.T) {
		i, err := NewSenderInterceptor()
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{SSRC: 1, RTPHeaderExtensions: []interceptor.RTPHeaderExtension{
			{
				URI: transportCCURI,
				ID:  1,
			},
		}}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, 1, len(pkts))
		tlcc, ok := pkts[0].(*rtcp.TransportLayerCC)
		assert.True(t, ok)
		assert.Equal(t, uint16(0), tlcc.PacketStatusCount)
		assert.Equal(t, uint8(0), tlcc.FbPktCount)
		assert.Equal(t, uint16(0), tlcc.BaseSequenceNumber)
		assert.Equal(t, uint32(0), tlcc.MediaSSRC)
		assert.Equal(t, uint32(0), tlcc.ReferenceTime)
		assert.Equal(t, 0, len(tlcc.RecvDeltas))
		assert.Equal(t, 0, len(tlcc.PacketChunks))
	})

	t.Run("after RTP packets", func(t *testing.T) {
		i, err := NewSenderInterceptor()
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{SSRC: 1, RTPHeaderExtensions: []interceptor.RTPHeaderExtension{
			{
				URI: transportCCURI,
				ID:  1,
			},
		}}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		for i := 0; i < 10; i++ {
			hdr := rtp.Header{}
			tcc, err := (&rtp.TransportCCExtension{TransportSequence: uint16(i)}).Marshal()
			assert.NoError(t, err)
			err = hdr.SetExtension(1, tcc)
			assert.NoError(t, err)
			stream.ReceiveRTP(&rtp.Packet{Header: hdr})
		}

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, 1, len(pkts))
		cc, ok := pkts[0].(*rtcp.TransportLayerCC)
		assert.True(t, ok)
		assert.Equal(t, uint32(1), cc.MediaSSRC)
		assert.Equal(t, uint16(0), cc.BaseSequenceNumber)
		assert.Equal(t, []rtcp.PacketStatusChunk{
			&rtcp.RunLengthChunk{
				PacketStatusSymbol: rtcp.TypeTCCPacketReceivedSmallDelta,
				RunLength:          10,
			},
		}, cc.PacketChunks)
	})

	t.Run("different delays between RTP packets", func(t *testing.T) {
		i, err := NewSenderInterceptor(
			SendInterval(500 * time.Millisecond),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{RTPHeaderExtensions: []interceptor.RTPHeaderExtension{
			{
				URI: transportCCURI,
				ID:  1,
			},
		}}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		delays := []int{0, 10, 100, 200}
		for i, d := range delays {
			time.Sleep(time.Duration(d) * time.Millisecond)

			hdr := rtp.Header{}
			tcc, err := (&rtp.TransportCCExtension{TransportSequence: uint16(i)}).Marshal()
			assert.NoError(t, err)
			err = hdr.SetExtension(1, tcc)
			assert.NoError(t, err)
			stream.ReceiveRTP(&rtp.Packet{Header: hdr})
		}

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, 1, len(pkts))
		cc, ok := pkts[0].(*rtcp.TransportLayerCC)
		assert.True(t, ok)
		assert.Equal(t, uint16(0), cc.BaseSequenceNumber)
		assert.Equal(t, []rtcp.PacketStatusChunk{
			&rtcp.StatusVectorChunk{
				SymbolSize: rtcp.TypeTCCSymbolSizeTwoBit,
				SymbolList: []uint16{
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketReceivedLargeDelta,
					rtcp.TypeTCCPacketReceivedLargeDelta,
				},
			},
		}, cc.PacketChunks)
	})

	t.Run("packet loss", func(t *testing.T) {
		i, err := NewSenderInterceptor(
			SendInterval(2 * time.Second),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{RTPHeaderExtensions: []interceptor.RTPHeaderExtension{
			{
				URI: transportCCURI,
				ID:  1,
			},
		}}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		seqNrToDelay := map[int]int{
			0:  0,
			1:  10,
			4:  100,
			8:  200,
			9:  20,
			10: 20,
			30: 300,
		}
		for _, i := range []int{0, 1, 4, 8, 9, 10, 30} {
			d := seqNrToDelay[i]
			time.Sleep(time.Duration(d) * time.Millisecond)

			hdr := rtp.Header{}
			tcc, err := (&rtp.TransportCCExtension{TransportSequence: uint16(i)}).Marshal()
			assert.NoError(t, err)
			err = hdr.SetExtension(1, tcc)
			assert.NoError(t, err)
			stream.ReceiveRTP(&rtp.Packet{Header: hdr})
		}

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, 1, len(pkts))
		cc, ok := pkts[0].(*rtcp.TransportLayerCC)
		assert.True(t, ok)
		assert.Equal(t, uint16(0), cc.BaseSequenceNumber)
		assert.Equal(t, []rtcp.PacketStatusChunk{
			&rtcp.StatusVectorChunk{
				SymbolSize: rtcp.TypeTCCSymbolSizeTwoBit,
				SymbolList: []uint16{
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketReceivedLargeDelta,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
				},
			},
			&rtcp.StatusVectorChunk{
				SymbolSize: rtcp.TypeTCCSymbolSizeTwoBit,
				SymbolList: []uint16{
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketReceivedLargeDelta,
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
				},
			},
			&rtcp.RunLengthChunk{
				PacketStatusSymbol: rtcp.TypeTCCPacketNotReceived,
				RunLength:          16,
			},
			&rtcp.RunLengthChunk{
				PacketStatusSymbol: rtcp.TypeTCCPacketReceivedLargeDelta,
				RunLength:          1,
			},
		}, cc.PacketChunks)
	})

	t.Run("overflow", func(t *testing.T) {
		i, err := NewSenderInterceptor(
			SendInterval(2 * time.Second),
		)
		assert.NoError(t, err)

		stream := test.NewMockStream(&interceptor.StreamInfo{RTPHeaderExtensions: []interceptor.RTPHeaderExtension{
			{
				URI: transportCCURI,
				ID:  1,
			},
		}}, i)
		defer func() {
			assert.NoError(t, stream.Close())
		}()

		for _, i := range []int{65530, 65534, 65535, 1, 2, 10} {
			hdr := rtp.Header{}
			tcc, err := (&rtp.TransportCCExtension{TransportSequence: uint16(i)}).Marshal()
			assert.NoError(t, err)
			err = hdr.SetExtension(1, tcc)
			assert.NoError(t, err)
			stream.ReceiveRTP(&rtp.Packet{Header: hdr})
		}

		pkts := <-stream.WrittenRTCP()
		assert.Equal(t, 1, len(pkts))
		cc, ok := pkts[0].(*rtcp.TransportLayerCC)
		assert.True(t, ok)
		assert.Equal(t, uint16(65530), cc.BaseSequenceNumber)
		assert.Equal(t, []rtcp.PacketStatusChunk{
			&rtcp.StatusVectorChunk{
				SymbolSize: rtcp.TypeTCCSymbolSizeOneBit,
				SymbolList: []uint16{
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
				},
			},
			&rtcp.StatusVectorChunk{
				SymbolSize: rtcp.TypeTCCSymbolSizeTwoBit,
				SymbolList: []uint16{
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketReceivedSmallDelta,
				},
			},
		}, cc.PacketChunks)
	})
}
