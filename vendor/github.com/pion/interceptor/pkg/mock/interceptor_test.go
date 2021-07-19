package mock

import (
	"sync/atomic"
	"testing"

	"github.com/pion/interceptor"
)

func TestInterceptor(t *testing.T) {
	dummyRTPWriter := &RTPWriter{}
	dummyRTPReader := &RTPReader{}
	dummyRTCPWriter := &RTCPWriter{}
	dummyRTCPReader := &RTCPReader{}
	dummyStreamInfo := &interceptor.StreamInfo{}

	t.Run("Default", func(t *testing.T) {
		i := &Interceptor{}

		if i.BindRTCPWriter(dummyRTCPWriter) != dummyRTCPWriter {
			t.Error("Default BindRTCPWriter should return given writer")
		}
		if i.BindRTCPReader(dummyRTCPReader) != dummyRTCPReader {
			t.Error("Default BindRTCPReader should return given reader")
		}
		if i.BindLocalStream(dummyStreamInfo, dummyRTPWriter) != dummyRTPWriter {
			t.Error("Default BindLocalStream should return given writer")
		}
		i.UnbindLocalStream(dummyStreamInfo)
		if i.BindRemoteStream(dummyStreamInfo, dummyRTPReader) != dummyRTPReader {
			t.Error("Default BindRemoteStream should return given reader")
		}
		i.UnbindRemoteStream(dummyStreamInfo)
		if i.Close() != nil {
			t.Error("Default Close should return nil")
		}
	})
	t.Run("Custom", func(t *testing.T) {
		var (
			cntBindRTCPReader     uint32
			cntBindRTCPWriter     uint32
			cntBindLocalStream    uint32
			cntUnbindLocalStream  uint32
			cntBindRemoteStream   uint32
			cntUnbindRemoteStream uint32
			cntClose              uint32
		)
		i := &Interceptor{
			BindRTCPReaderFn: func(reader interceptor.RTCPReader) interceptor.RTCPReader {
				atomic.AddUint32(&cntBindRTCPReader, 1)
				return reader
			},
			BindRTCPWriterFn: func(writer interceptor.RTCPWriter) interceptor.RTCPWriter {
				atomic.AddUint32(&cntBindRTCPWriter, 1)
				return writer
			},
			BindLocalStreamFn: func(i *interceptor.StreamInfo, writer interceptor.RTPWriter) interceptor.RTPWriter {
				atomic.AddUint32(&cntBindLocalStream, 1)
				return writer
			},
			UnbindLocalStreamFn: func(i *interceptor.StreamInfo) {
				atomic.AddUint32(&cntUnbindLocalStream, 1)
			},
			BindRemoteStreamFn: func(i *interceptor.StreamInfo, reader interceptor.RTPReader) interceptor.RTPReader {
				atomic.AddUint32(&cntBindRemoteStream, 1)
				return reader
			},
			UnbindRemoteStreamFn: func(i *interceptor.StreamInfo) {
				atomic.AddUint32(&cntUnbindRemoteStream, 1)
			},
			CloseFn: func() error {
				atomic.AddUint32(&cntClose, 1)
				return nil
			},
		}

		if i.BindRTCPWriter(dummyRTCPWriter) != dummyRTCPWriter {
			t.Error("Mocked BindRTCPWriter should return given writer")
		}
		if i.BindRTCPReader(dummyRTCPReader) != dummyRTCPReader {
			t.Error("Mocked BindRTCPReader should return given reader")
		}
		if i.BindLocalStream(dummyStreamInfo, dummyRTPWriter) != dummyRTPWriter {
			t.Error("Mocked BindLocalStream should return given writer")
		}
		i.UnbindLocalStream(dummyStreamInfo)
		if i.BindRemoteStream(dummyStreamInfo, dummyRTPReader) != dummyRTPReader {
			t.Error("Mocked BindRemoteStream should return given reader")
		}
		i.UnbindRemoteStream(dummyStreamInfo)
		if i.Close() != nil {
			t.Error("Mocked Close should return nil")
		}

		if cnt := atomic.LoadUint32(&cntBindRTCPWriter); cnt != 1 {
			t.Errorf("BindRTCPWriterFn is expected to be called once, but called %d times", cnt)
		}
		if cnt := atomic.LoadUint32(&cntBindRTCPReader); cnt != 1 {
			t.Errorf("BindRTCPReaderFn is expected to be called once, but called %d times", cnt)
		}
		if cnt := atomic.LoadUint32(&cntBindLocalStream); cnt != 1 {
			t.Errorf("BindLocalStreamFn is expected to be called once, but called %d times", cnt)
		}
		if cnt := atomic.LoadUint32(&cntUnbindLocalStream); cnt != 1 {
			t.Errorf("UnbindLocalStreamFn is expected to be called once, but called %d times", cnt)
		}
		if cnt := atomic.LoadUint32(&cntBindRemoteStream); cnt != 1 {
			t.Errorf("BindRemoteStreamFn is expected to be called once, but called %d times", cnt)
		}
		if cnt := atomic.LoadUint32(&cntUnbindRemoteStream); cnt != 1 {
			t.Errorf("UnbindRemoteStreamFn is expected to be called once, but called %d times", cnt)
		}
		if cnt := atomic.LoadUint32(&cntClose); cnt != 1 {
			t.Errorf("CloseFn is expected to be called once, but called %d times", cnt)
		}
	})
}
