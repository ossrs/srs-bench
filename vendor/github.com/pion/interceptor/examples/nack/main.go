package main

import (
	"fmt"
	"log"
	"net"
	"time"

	"github.com/pion/interceptor"
	"github.com/pion/interceptor/pkg/nack"
	"github.com/pion/rtcp"
	"github.com/pion/rtp"
)

const (
	listenPort = 6420
	mtu        = 1500
	ssrc       = 5000
)

func main() {
	go sendRoutine()
	receiveRoutine()
}

func receiveRoutine() {
	serverAddr, err := net.ResolveUDPAddr("udp4", fmt.Sprintf("127.0.0.1:%d", listenPort))
	if err != nil {
		panic(err)
	}

	conn, err := net.ListenUDP("udp4", serverAddr)
	if err != nil {
		panic(err)
	}

	// Create NACK Generator
	generator, err := nack.NewGeneratorInterceptor()
	if err != nil {
		panic(err)
	}

	// Create our interceptor chain with just a NACK Generator
	chain := interceptor.NewChain([]interceptor.Interceptor{generator})

	// Create the writer just for a single SSRC stream
	// this is a callback that is fired everytime a RTP packet is ready to be sent
	streamReader := chain.BindRemoteStream(&interceptor.StreamInfo{
		SSRC:         ssrc,
		RTCPFeedback: []interceptor.RTCPFeedback{{Type: "nack", Parameter: ""}},
	}, interceptor.RTPReaderFunc(func(b []byte, _ interceptor.Attributes) (int, interceptor.Attributes, error) { return len(b), nil, nil }))

	for rtcpBound, buffer := false, make([]byte, mtu); ; {
		i, addr, err := conn.ReadFrom(buffer)
		if err != nil {
			panic(err)
		}

		log.Println("Received RTP")

		if _, _, err := streamReader.Read(buffer[:i], nil); err != nil {
			panic(err)
		}

		// Set the interceptor wide RTCP Writer
		// this is a callback that is fired everytime a RTCP packet is ready to be sent
		if !rtcpBound {
			chain.BindRTCPWriter(interceptor.RTCPWriterFunc(func(pkts []rtcp.Packet, _ interceptor.Attributes) (int, error) {
				buf, err := rtcp.Marshal(pkts)
				if err != nil {
					return 0, err
				}

				return conn.WriteTo(buf, addr)
			}))

			rtcpBound = true
		}
	}
}

func sendRoutine() {
	// Dial our UDP listener that we create in receiveRoutine
	serverAddr, err := net.ResolveUDPAddr("udp4", fmt.Sprintf("127.0.0.1:%d", listenPort))
	if err != nil {
		panic(err)
	}

	conn, err := net.DialUDP("udp4", nil, serverAddr)
	if err != nil {
		panic(err)
	}

	// Create NACK Responder
	responder, err := nack.NewResponderInterceptor()
	if err != nil {
		panic(err)
	}

	// Create our interceptor chain with just a NACK Responder.
	chain := interceptor.NewChain([]interceptor.Interceptor{responder})

	// Set the interceptor wide RTCP Reader
	// this is a handle to send NACKs back into the interceptor.
	rtcpWriter := chain.BindRTCPReader(interceptor.RTCPReaderFunc(func(in []byte, _ interceptor.Attributes) (int, interceptor.Attributes, error) {
		return len(in), nil, nil
	}))

	// Create the writer just for a single SSRC stream
	// this is a callback that is fired everytime a RTP packet is ready to be sent
	streamWriter := chain.BindLocalStream(&interceptor.StreamInfo{
		SSRC:         ssrc,
		RTCPFeedback: []interceptor.RTCPFeedback{{Type: "nack", Parameter: ""}},
	}, interceptor.RTPWriterFunc(func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
		headerBuf, err := header.Marshal()
		if err != nil {
			panic(err)
		}

		return conn.Write(append(headerBuf, payload...))
	}))

	// Read RTCP packets sent by receiver and pass into Interceptor
	go func() {
		for rtcpBuf := make([]byte, mtu); ; {
			i, err := conn.Read(rtcpBuf)
			if err != nil {
				panic(err)
			}

			log.Println("Received NACK")

			if _, _, err = rtcpWriter.Read(rtcpBuf[:i], nil); err != nil {
				panic(err)
			}
		}
	}()

	for sequenceNumber := uint16(0); ; sequenceNumber++ {
		// Send a RTP packet with a Payload of 0x0, 0x1, 0x2
		if _, err := streamWriter.Write(&rtp.Header{
			Version:        2,
			SSRC:           ssrc,
			SequenceNumber: sequenceNumber,
		}, []byte{0x0, 0x1, 0x2}, nil); err != nil {
			fmt.Println(err)
		}

		time.Sleep(time.Millisecond * 200)
	}
}
