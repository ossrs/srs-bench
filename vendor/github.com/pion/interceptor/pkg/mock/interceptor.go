// Package mock provides mock Interceptor for testing.
package mock

import (
	"github.com/pion/interceptor"
	"github.com/pion/rtcp"
	"github.com/pion/rtp"
)

// Interceptor is an mock Interceptor fot testing.
type Interceptor struct {
	BindRTCPReaderFn     func(reader interceptor.RTCPReader) interceptor.RTCPReader
	BindRTCPWriterFn     func(writer interceptor.RTCPWriter) interceptor.RTCPWriter
	BindLocalStreamFn    func(i *interceptor.StreamInfo, writer interceptor.RTPWriter) interceptor.RTPWriter
	UnbindLocalStreamFn  func(i *interceptor.StreamInfo)
	BindRemoteStreamFn   func(i *interceptor.StreamInfo, reader interceptor.RTPReader) interceptor.RTPReader
	UnbindRemoteStreamFn func(i *interceptor.StreamInfo)
	CloseFn              func() error
}

// BindRTCPReader implements Interceptor.
func (i *Interceptor) BindRTCPReader(reader interceptor.RTCPReader) interceptor.RTCPReader {
	if i.BindRTCPReaderFn != nil {
		return i.BindRTCPReaderFn(reader)
	}
	return reader
}

// BindRTCPWriter implements Interceptor.
func (i *Interceptor) BindRTCPWriter(writer interceptor.RTCPWriter) interceptor.RTCPWriter {
	if i.BindRTCPWriterFn != nil {
		return i.BindRTCPWriterFn(writer)
	}
	return writer
}

// BindLocalStream implements Interceptor.
func (i *Interceptor) BindLocalStream(info *interceptor.StreamInfo, writer interceptor.RTPWriter) interceptor.RTPWriter {
	if i.BindLocalStreamFn != nil {
		return i.BindLocalStreamFn(info, writer)
	}
	return writer
}

// UnbindLocalStream implements Interceptor.
func (i *Interceptor) UnbindLocalStream(info *interceptor.StreamInfo) {
	if i.UnbindLocalStreamFn != nil {
		i.UnbindLocalStreamFn(info)
	}
}

// BindRemoteStream implements Interceptor.
func (i *Interceptor) BindRemoteStream(info *interceptor.StreamInfo, reader interceptor.RTPReader) interceptor.RTPReader {
	if i.BindRemoteStreamFn != nil {
		return i.BindRemoteStreamFn(info, reader)
	}
	return reader
}

// UnbindRemoteStream implements Interceptor.
func (i *Interceptor) UnbindRemoteStream(info *interceptor.StreamInfo) {
	if i.UnbindRemoteStreamFn != nil {
		i.UnbindRemoteStreamFn(info)
	}
}

// Close implements Interceptor.
func (i *Interceptor) Close() error {
	if i.CloseFn != nil {
		return i.CloseFn()
	}
	return nil
}

// RTPWriter is a mock RTPWriter.
type RTPWriter struct {
	WriteFn func(*rtp.Header, []byte, interceptor.Attributes) (int, error)
}

// Write implements RTPWriter.
func (w *RTPWriter) Write(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
	return w.WriteFn(header, payload, attributes)
}

// RTPReader is a mock RTPReader.
type RTPReader struct {
	ReadFn func([]byte, interceptor.Attributes) (int, interceptor.Attributes, error)
}

// Read implements RTPReader.
func (r *RTPReader) Read(b []byte, attributes interceptor.Attributes) (int, interceptor.Attributes, error) {
	return r.ReadFn(b, attributes)
}

// RTCPWriter is a mock RTCPWriter.
type RTCPWriter struct {
	WriteFn func([]rtcp.Packet, interceptor.Attributes) (int, error)
}

// Write implements RTCPWriter.
func (w *RTCPWriter) Write(pkts []rtcp.Packet, attributes interceptor.Attributes) (int, error) {
	return w.WriteFn(pkts, attributes)
}

// RTCPReader is a mock RTCPReader.
type RTCPReader struct {
	ReadFn func([]byte, interceptor.Attributes) (int, interceptor.Attributes, error)
}

// Read implements RTCPReader.
func (r *RTCPReader) Read(b []byte, attributes interceptor.Attributes) (int, interceptor.Attributes, error) {
	return r.ReadFn(b, attributes)
}
