package srs

import (
	"github.com/pion/interceptor"
	"github.com/pion/rtcp"
	"github.com/pion/rtp"
)

// Common RTP packet interceptor for benchmark.
// @remark Should never merge with rtcpInterceptor, because they has the same Write interface.
type rtpInterceptor struct {
	localInfo  *interceptor.StreamInfo
	remoteInfo *interceptor.StreamInfo
	// If rtpReader is nil, use the default next one to read.
	rtpReader     interceptor.RTPReaderFunc
	nextRTPReader interceptor.RTPReader
	// If rtpWriter is nil, use the default next one to write.
	rtpWriter     interceptor.RTPWriterFunc
	nextRTPWriter interceptor.RTPWriter
}

func (v *rtpInterceptor) BindRTCPReader(reader interceptor.RTCPReader) interceptor.RTCPReader {
	return reader // Ignore RTCP
}

func (v *rtpInterceptor) BindRTCPWriter(writer interceptor.RTCPWriter) interceptor.RTCPWriter {
	return writer // Ignore RTCP
}

func (v *rtpInterceptor) BindLocalStream(info *interceptor.StreamInfo, writer interceptor.RTPWriter) interceptor.RTPWriter {
	if v.localInfo != nil {
		return writer // Only handle one stream.
	}

	v.localInfo = info
	v.nextRTPWriter = writer
	return v // Handle all RTP
}

func (v *rtpInterceptor) Write(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
	if v.rtpWriter != nil {
		return v.rtpWriter(header, payload, attributes)
	}
	return v.nextRTPWriter.Write(header, payload, attributes)
}

func (v *rtpInterceptor) UnbindLocalStream(info *interceptor.StreamInfo) {
	if v.localInfo == nil || v.localInfo.ID != info.ID {
		return
	}
	v.localInfo = nil // Reset the interceptor.
}

func (v *rtpInterceptor) BindRemoteStream(info *interceptor.StreamInfo, reader interceptor.RTPReader) interceptor.RTPReader {
	if v.remoteInfo != nil {
		return reader // Only handle one stream.
	}

	v.nextRTPReader = reader
	return v // Handle all RTP
}

func (v *rtpInterceptor) Read(b []byte, a interceptor.Attributes) (int, interceptor.Attributes, error) {
	if v.rtpReader != nil {
		return v.rtpReader(b, a)
	}
	return v.nextRTPReader.Read(b, a)
}

func (v *rtpInterceptor) UnbindRemoteStream(info *interceptor.StreamInfo) {
	if v.remoteInfo == nil || v.remoteInfo.ID != info.ID {
		return
	}
	v.remoteInfo = nil
}

func (v *rtpInterceptor) Close() error {
	return nil
}

// Common RTCP packet interceptor for benchmark.
// @remark Should never merge with rtpInterceptor, because they has the same Write interface.
type rtcpInterceptor struct {
	// If rtcpReader is nil, use the default next one to read.
	rtcpReader     interceptor.RTCPReaderFunc
	nextRTCPReader interceptor.RTCPReader
	// If rtcpWriter is nil, use the default next one to write.
	rtcpWriter     interceptor.RTCPWriterFunc
	nextRTCPWriter interceptor.RTCPWriter
}

func (v *rtcpInterceptor) BindRTCPReader(reader interceptor.RTCPReader) interceptor.RTCPReader {
	v.nextRTCPReader = reader
	return v // Handle all RTCP
}

func (v *rtcpInterceptor) Read(b []byte, a interceptor.Attributes) (int, interceptor.Attributes, error) {
	if v.rtcpReader != nil {
		return v.rtcpReader(b, a)
	}
	return v.nextRTCPReader.Read(b, a)
}

func (v *rtcpInterceptor) BindRTCPWriter(writer interceptor.RTCPWriter) interceptor.RTCPWriter {
	v.nextRTCPWriter = writer
	return v // Handle all RTCP
}

func (v *rtcpInterceptor) Write(pkts []rtcp.Packet, attributes interceptor.Attributes) (int, error) {
	if v.rtcpWriter != nil {
		return v.rtcpWriter(pkts, attributes)
	}
	return v.nextRTCPWriter.Write(pkts, attributes)
}

func (v *rtcpInterceptor) BindLocalStream(info *interceptor.StreamInfo, writer interceptor.RTPWriter) interceptor.RTPWriter {
	return writer // Ignore RTP
}

func (v *rtcpInterceptor) UnbindLocalStream(info *interceptor.StreamInfo) {
}

func (v *rtcpInterceptor) BindRemoteStream(info *interceptor.StreamInfo, reader interceptor.RTPReader) interceptor.RTPReader {
	return reader // Ignore RTP
}

func (v *rtcpInterceptor) UnbindRemoteStream(info *interceptor.StreamInfo) {
}

func (v *rtcpInterceptor) Close() error {
	return nil
}
