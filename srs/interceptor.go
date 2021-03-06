package srs

import (
	"github.com/pion/interceptor"
	"github.com/pion/rtcp"
	"github.com/pion/rtp"
)

// Common RTP packet interceptor for benchmark.
// @remark Should never merge with RTCPInterceptor, because they has the same Write interface.
type RTPInterceptor struct {
	localInfo  *interceptor.StreamInfo
	remoteInfo *interceptor.StreamInfo
	// If rtpReader is nil, use the default next one to read.
	rtpReader     interceptor.RTPReaderFunc
	nextRTPReader interceptor.RTPReader
	// If rtpWriter is nil, use the default next one to write.
	rtpWriter     interceptor.RTPWriterFunc
	nextRTPWriter interceptor.RTPWriter
}

func (v *RTPInterceptor) BindRTCPReader(reader interceptor.RTCPReader) interceptor.RTCPReader {
	return reader // Ignore RTCP
}

func (v *RTPInterceptor) BindRTCPWriter(writer interceptor.RTCPWriter) interceptor.RTCPWriter {
	return writer // Ignore RTCP
}

func (v *RTPInterceptor) BindLocalStream(info *interceptor.StreamInfo, writer interceptor.RTPWriter) interceptor.RTPWriter {
	if v.localInfo != nil {
		return writer // Only handle one stream.
	}

	v.localInfo = info
	v.nextRTPWriter = writer
	return v // Handle all RTP
}

func (v *RTPInterceptor) Write(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
	if v.rtpWriter != nil {
		return v.rtpWriter(header, payload, attributes)
	}
	return v.nextRTPWriter.Write(header, payload, attributes)
}

func (v *RTPInterceptor) UnbindLocalStream(info *interceptor.StreamInfo) {
	if v.localInfo == nil || v.localInfo.ID != info.ID {
		return
	}
	v.localInfo = nil // Reset the interceptor.
}

func (v *RTPInterceptor) BindRemoteStream(info *interceptor.StreamInfo, reader interceptor.RTPReader) interceptor.RTPReader {
	if v.remoteInfo != nil {
		return reader // Only handle one stream.
	}

	v.nextRTPReader = reader
	return v // Handle all RTP
}

func (v *RTPInterceptor) Read(b []byte, a interceptor.Attributes) (int, interceptor.Attributes, error) {
	if v.rtpReader != nil {
		return v.rtpReader(b, a)
	}
	return v.nextRTPReader.Read(b, a)
}

func (v *RTPInterceptor) UnbindRemoteStream(info *interceptor.StreamInfo) {
	if v.remoteInfo == nil || v.remoteInfo.ID != info.ID {
		return
	}
	v.remoteInfo = nil
}

func (v *RTPInterceptor) Close() error {
	return nil
}

// Common RTCP packet interceptor for benchmark.
// @remark Should never merge with RTPInterceptor, because they has the same Write interface.
type RTCPInterceptor struct {
	// If rtcpReader is nil, use the default next one to read.
	rtcpReader     interceptor.RTCPReaderFunc
	nextRTCPReader interceptor.RTCPReader
	// If rtcpWriter is nil, use the default next one to write.
	rtcpWriter     interceptor.RTCPWriterFunc
	nextRTCPWriter interceptor.RTCPWriter
}

func (v *RTCPInterceptor) BindRTCPReader(reader interceptor.RTCPReader) interceptor.RTCPReader {
	v.nextRTCPReader = reader
	return v // Handle all RTCP
}

func (v *RTCPInterceptor) Read(b []byte, a interceptor.Attributes) (int, interceptor.Attributes, error) {
	if v.rtcpReader != nil {
		return v.rtcpReader(b, a)
	}
	return v.nextRTCPReader.Read(b, a)
}

func (v *RTCPInterceptor) BindRTCPWriter(writer interceptor.RTCPWriter) interceptor.RTCPWriter {
	v.nextRTCPWriter = writer
	return v // Handle all RTCP
}

func (v *RTCPInterceptor) Write(pkts []rtcp.Packet, attributes interceptor.Attributes) (int, error) {
	if v.rtcpWriter != nil {
		return v.rtcpWriter(pkts, attributes)
	}
	return v.nextRTCPWriter.Write(pkts, attributes)
}

func (v *RTCPInterceptor) BindLocalStream(info *interceptor.StreamInfo, writer interceptor.RTPWriter) interceptor.RTPWriter {
	return writer // Ignore RTP
}

func (v *RTCPInterceptor) UnbindLocalStream(info *interceptor.StreamInfo) {
}

func (v *RTCPInterceptor) BindRemoteStream(info *interceptor.StreamInfo, reader interceptor.RTPReader) interceptor.RTPReader {
	return reader // Ignore RTP
}

func (v *RTCPInterceptor) UnbindRemoteStream(info *interceptor.StreamInfo) {
}

func (v *RTCPInterceptor) Close() error {
	return nil
}
