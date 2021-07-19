package twcc

import (
	"testing"

	"github.com/pion/rtcp"
	"github.com/stretchr/testify/assert"
)

func Test_chunk_add(t *testing.T) {
	t.Run("fill with not received", func(t *testing.T) {
		c := &chunk{}

		for i := 0; i < maxRunLengthCap; i++ {
			assert.True(t, c.canAdd(rtcp.TypeTCCPacketNotReceived), i)
			c.add(rtcp.TypeTCCPacketNotReceived)
		}
		assert.Equal(t, make([]uint16, maxRunLengthCap), c.deltas)
		assert.False(t, c.hasDifferentTypes)

		assert.False(t, c.canAdd(rtcp.TypeTCCPacketNotReceived))
		assert.False(t, c.canAdd(rtcp.TypeTCCPacketReceivedSmallDelta))
		assert.False(t, c.canAdd(rtcp.TypeTCCPacketReceivedLargeDelta))

		statusChunk := c.encode()
		assert.IsType(t, &rtcp.RunLengthChunk{}, statusChunk)

		buf, err := statusChunk.Marshal()
		assert.NoError(t, err)
		assert.Equal(t, []byte{0x1f, 0xff}, buf)
	})

	t.Run("fill with small delta", func(t *testing.T) {
		c := &chunk{}

		for i := 0; i < maxOneBitCap; i++ {
			assert.True(t, c.canAdd(rtcp.TypeTCCPacketReceivedSmallDelta), i)
			c.add(rtcp.TypeTCCPacketReceivedSmallDelta)
		}

		assert.Equal(t, c.deltas, []uint16{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1})
		assert.False(t, c.hasDifferentTypes)

		assert.False(t, c.canAdd(rtcp.TypeTCCPacketReceivedLargeDelta))
		assert.False(t, c.canAdd(rtcp.TypeTCCPacketNotReceived))

		statusChunk := c.encode()
		assert.IsType(t, &rtcp.RunLengthChunk{}, statusChunk)

		buf, err := statusChunk.Marshal()
		assert.NoError(t, err)
		assert.Equal(t, []byte{0x20, 0xe}, buf)
	})

	t.Run("fill with large delta", func(t *testing.T) {
		c := &chunk{}

		for i := 0; i < maxTwoBitCap; i++ {
			assert.True(t, c.canAdd(rtcp.TypeTCCPacketReceivedLargeDelta), i)
			c.add(rtcp.TypeTCCPacketReceivedLargeDelta)
		}

		assert.Equal(t, c.deltas, []uint16{2, 2, 2, 2, 2, 2, 2})
		assert.True(t, c.hasLargeDelta)
		assert.False(t, c.hasDifferentTypes)

		assert.False(t, c.canAdd(rtcp.TypeTCCPacketReceivedSmallDelta))
		assert.False(t, c.canAdd(rtcp.TypeTCCPacketNotReceived))

		statusChunk := c.encode()
		assert.IsType(t, &rtcp.RunLengthChunk{}, statusChunk)

		buf, err := statusChunk.Marshal()
		assert.NoError(t, err)
		assert.Equal(t, []byte{0x40, 0x7}, buf)
	})

	t.Run("fill with different types", func(t *testing.T) {
		c := &chunk{}

		assert.True(t, c.canAdd(rtcp.TypeTCCPacketReceivedSmallDelta))
		c.add(rtcp.TypeTCCPacketReceivedSmallDelta)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketReceivedSmallDelta))
		c.add(rtcp.TypeTCCPacketReceivedSmallDelta)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketReceivedSmallDelta))
		c.add(rtcp.TypeTCCPacketReceivedSmallDelta)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketReceivedSmallDelta))
		c.add(rtcp.TypeTCCPacketReceivedSmallDelta)

		assert.True(t, c.canAdd(rtcp.TypeTCCPacketReceivedLargeDelta))
		c.add(rtcp.TypeTCCPacketReceivedLargeDelta)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketReceivedLargeDelta))
		c.add(rtcp.TypeTCCPacketReceivedLargeDelta)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketReceivedLargeDelta))
		c.add(rtcp.TypeTCCPacketReceivedLargeDelta)

		assert.False(t, c.canAdd(rtcp.TypeTCCPacketReceivedLargeDelta))

		statusChunk := c.encode()
		assert.IsType(t, &rtcp.StatusVectorChunk{}, statusChunk)

		buf, err := statusChunk.Marshal()
		assert.NoError(t, err)
		assert.Equal(t, []byte{0xd5, 0x6a}, buf)
	})

	t.Run("overfill and encode", func(t *testing.T) {
		c := chunk{}

		assert.True(t, c.canAdd(rtcp.TypeTCCPacketReceivedSmallDelta))
		c.add(rtcp.TypeTCCPacketReceivedSmallDelta)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketNotReceived))
		c.add(rtcp.TypeTCCPacketNotReceived)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketNotReceived))
		c.add(rtcp.TypeTCCPacketNotReceived)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketNotReceived))
		c.add(rtcp.TypeTCCPacketNotReceived)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketNotReceived))
		c.add(rtcp.TypeTCCPacketNotReceived)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketNotReceived))
		c.add(rtcp.TypeTCCPacketNotReceived)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketNotReceived))
		c.add(rtcp.TypeTCCPacketNotReceived)
		assert.True(t, c.canAdd(rtcp.TypeTCCPacketNotReceived))
		c.add(rtcp.TypeTCCPacketNotReceived)

		assert.False(t, c.canAdd(rtcp.TypeTCCPacketReceivedLargeDelta))

		statusChunk1 := c.encode()
		assert.IsType(t, &rtcp.StatusVectorChunk{}, statusChunk1)
		assert.Equal(t, 1, len(c.deltas))

		assert.True(t, c.canAdd(rtcp.TypeTCCPacketReceivedLargeDelta))
		c.add(rtcp.TypeTCCPacketReceivedLargeDelta)

		statusChunk2 := c.encode()
		assert.IsType(t, &rtcp.StatusVectorChunk{}, statusChunk2)

		assert.Equal(t, 0, len(c.deltas))

		assert.Equal(t, &rtcp.StatusVectorChunk{
			SymbolSize: rtcp.TypeTCCSymbolSizeTwoBit,
			SymbolList: []uint16{rtcp.TypeTCCPacketNotReceived, rtcp.TypeTCCPacketReceivedLargeDelta},
		}, statusChunk2)
	})
}

func Test_feedback(t *testing.T) {
	t.Run("add simple", func(t *testing.T) {
		f := feedback{}

		got := f.addReceived(0, 10)

		assert.True(t, got)
	})

	t.Run("add too large", func(t *testing.T) {
		f := feedback{}

		assert.False(t, f.addReceived(12, 8200*1000*250))
	})

	t.Run("add received 1", func(t *testing.T) {
		f := &feedback{}
		f.setBase(1, 1000*1000)

		got := f.addReceived(1, 1023*1000)

		assert.True(t, got)
		assert.Equal(t, uint16(2), f.nextSeqNr)
		assert.Equal(t, int64(15), f.refTimestamp64MS)

		got = f.addReceived(4, 1086*1000)
		assert.True(t, got)
		assert.Equal(t, uint16(5), f.nextSeqNr)
		assert.Equal(t, int64(15), f.refTimestamp64MS)

		assert.True(t, f.lastChunk.hasDifferentTypes)
		assert.Equal(t, 4, len(f.lastChunk.deltas))
		assert.NotContains(t, f.lastChunk.deltas, rtcp.TypeTCCPacketReceivedLargeDelta)
	})

	t.Run("add received 2", func(t *testing.T) {
		f := newFeedback(0, 0, 0)
		f.setBase(5, 320*1000)

		got := f.addReceived(5, 320*1000)
		assert.True(t, got)
		got = f.addReceived(7, 448*1000)
		assert.True(t, got)
		got = f.addReceived(8, 512*1000)
		assert.True(t, got)
		got = f.addReceived(11, 768*1000)
		assert.True(t, got)

		pkt := f.getRTCP()

		assert.True(t, pkt.Header.Padding)
		assert.Equal(t, uint16(7), pkt.Header.Length)
		assert.Equal(t, uint16(5), pkt.BaseSequenceNumber)
		assert.Equal(t, uint16(7), pkt.PacketStatusCount)
		assert.Equal(t, uint32(5), pkt.ReferenceTime)
		assert.Equal(t, uint8(0), pkt.FbPktCount)
		assert.Equal(t, 1, len(pkt.PacketChunks))

		assert.Equal(t, []rtcp.PacketStatusChunk{&rtcp.StatusVectorChunk{
			SymbolSize: rtcp.TypeTCCSymbolSizeTwoBit,
			SymbolList: []uint16{
				rtcp.TypeTCCPacketReceivedSmallDelta,
				rtcp.TypeTCCPacketNotReceived,
				rtcp.TypeTCCPacketReceivedLargeDelta,
				rtcp.TypeTCCPacketReceivedLargeDelta,
				rtcp.TypeTCCPacketNotReceived,
				rtcp.TypeTCCPacketNotReceived,
				rtcp.TypeTCCPacketReceivedLargeDelta,
			},
		}}, pkt.PacketChunks)

		expectedDeltas := []*rtcp.RecvDelta{
			{
				Type:  rtcp.TypeTCCPacketReceivedSmallDelta,
				Delta: 0,
			},
			{
				Type:  rtcp.TypeTCCPacketReceivedLargeDelta,
				Delta: 0x0200,
			},
			{
				Type:  rtcp.TypeTCCPacketReceivedLargeDelta,
				Delta: 0x0100,
			},
			{
				Type:  rtcp.TypeTCCPacketReceivedLargeDelta,
				Delta: 0x0400,
			},
		}
		assert.Equal(t, len(expectedDeltas), len(pkt.RecvDeltas))
		for i, d := range expectedDeltas {
			assert.Equal(t, d, pkt.RecvDeltas[i])
		}
	})

	t.Run("add received wrapped sequence number", func(t *testing.T) {
		f := newFeedback(0, 0, 0)
		f.setBase(65535, 320*1000)

		got := f.addReceived(65535, 320*1000)
		assert.True(t, got)
		got = f.addReceived(7, 448*1000)
		assert.True(t, got)
		got = f.addReceived(8, 512*1000)
		assert.True(t, got)
		got = f.addReceived(11, 768*1000)
		assert.True(t, got)

		pkt := f.getRTCP()

		assert.True(t, pkt.Header.Padding)
		assert.Equal(t, uint16(7), pkt.Header.Length)
		assert.Equal(t, uint16(65535), pkt.BaseSequenceNumber)
		assert.Equal(t, uint16(13), pkt.PacketStatusCount)
		assert.Equal(t, uint32(5), pkt.ReferenceTime)
		assert.Equal(t, uint8(0), pkt.FbPktCount)
		assert.Equal(t, 2, len(pkt.PacketChunks))

		assert.Equal(t, []rtcp.PacketStatusChunk{
			&rtcp.StatusVectorChunk{
				SymbolSize: rtcp.TypeTCCSymbolSizeTwoBit,
				SymbolList: []uint16{
					rtcp.TypeTCCPacketReceivedSmallDelta,
					rtcp.TypeTCCPacketNotReceived,
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
					rtcp.TypeTCCPacketReceivedLargeDelta,
					rtcp.TypeTCCPacketReceivedLargeDelta,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketNotReceived,
					rtcp.TypeTCCPacketReceivedLargeDelta,
				},
			},
		}, pkt.PacketChunks)

		expectedDeltas := []*rtcp.RecvDelta{
			{
				Type:  rtcp.TypeTCCPacketReceivedSmallDelta,
				Delta: 0,
			},
			{
				Type:  rtcp.TypeTCCPacketReceivedLargeDelta,
				Delta: 0x0200,
			},
			{
				Type:  rtcp.TypeTCCPacketReceivedLargeDelta,
				Delta: 0x0100,
			},
			{
				Type:  rtcp.TypeTCCPacketReceivedLargeDelta,
				Delta: 0x0400,
			},
		}
		assert.Equal(t, len(expectedDeltas), len(pkt.RecvDeltas))
		for i, d := range expectedDeltas {
			assert.Equal(t, d, pkt.RecvDeltas[i])
		}
	})

	t.Run("get RTCP", func(t *testing.T) {
		testcases := []struct {
			arrivalTS     int64
			seqNr         uint16
			wantRefTime   uint32
			wantBaseSeqNr uint16
		}{
			{320, 1, 5, 1},
			{1000, 2, 15, 2},
		}
		for _, tt := range testcases {
			tt := tt

			t.Run("set correct base seq and time", func(t *testing.T) {
				f := newFeedback(0, 0, 0)
				f.setBase(tt.seqNr, tt.arrivalTS*1000)

				got := f.getRTCP()
				assert.Equal(t, tt.wantRefTime, got.ReferenceTime)
				assert.Equal(t, tt.wantBaseSeqNr, got.BaseSequenceNumber)
			})
		}
	})
}
