package twcc

import (
	"sync/atomic"

	"github.com/pion/interceptor"
	"github.com/pion/rtp"
)

// HeaderExtensionInterceptor adds transport wide sequence numbers as header extension to each RTP packet
type HeaderExtensionInterceptor struct {
	interceptor.NoOp
	nextSequenceNr uint32
}

// NewHeaderExtensionInterceptor returns a HeaderExtensionInterceptor
func NewHeaderExtensionInterceptor() (*HeaderExtensionInterceptor, error) {
	return &HeaderExtensionInterceptor{}, nil
}

const transportCCURI = "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"

// BindLocalStream returns a writer that adds a rtp.TransportCCExtension
// header with increasing sequence numbers to each outgoing packet.
func (h *HeaderExtensionInterceptor) BindLocalStream(info *interceptor.StreamInfo, writer interceptor.RTPWriter) interceptor.RTPWriter {
	var hdrExtID uint8
	for _, e := range info.RTPHeaderExtensions {
		if e.URI == transportCCURI {
			hdrExtID = uint8(e.ID)
			break
		}
	}
	if hdrExtID == 0 { // Don't add header extension if ID is 0, because 0 is an invalid extension ID
		return writer
	}
	return interceptor.RTPWriterFunc(func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
		seqNr := atomic.AddUint32(&h.nextSequenceNr, 1) - 1

		tcc, err := (&rtp.TransportCCExtension{TransportSequence: uint16(seqNr)}).Marshal()
		if err != nil {
			return 0, err
		}
		err = header.SetExtension(hdrExtID, tcc)
		if err != nil {
			return 0, err
		}
		return writer.Write(header, payload, attributes)
	})
}
