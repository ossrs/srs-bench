// The MIT License (MIT)
//
// Copyright (c) 2021 srs-bench(ossrs)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
package srs

import (
	"context"
	"fmt"
	"github.com/ossrs/go-oryx-lib/logger"
	"github.com/pion/interceptor"
	"github.com/pion/rtcp"
	"github.com/pion/rtp"
	"github.com/pion/transport/vnet"
	"math/rand"
	"os"
	"sync"
	"testing"
	"time"
)

func TestRTCServerPublishPlay(t *testing.T) {
	ctx := logger.WithContext(context.Background())
	ctx, cancel := context.WithTimeout(ctx, time.Duration(*srsTimeout)*time.Millisecond)

	var r0, r1, r2, r3 error
	defer func(ctx context.Context) {
		if err := filterTestError(ctx.Err(), r0, r1, r2, r3); err != nil {
			t.Errorf("Fail for err %+v", err)
		} else {
			logger.Tf(ctx, "test done with err %+v", err)
		}
	}(ctx)

	var wg sync.WaitGroup
	defer wg.Wait()

	// The event notify.
	var thePublisher *TestPublisher
	var thePlayer *TestPlayer
	mainReady, mainReadyCancel := context.WithCancel(context.Background())
	publishReady, publishReadyCancel := context.WithCancel(context.Background())

	// Objects init.
	wg.Add(1)
	go func() {
		defer wg.Done()
		defer cancel()

		doInit := func() error {
			playOK := *srsPlayOKPackets
			vnetClientIP := *srsVnetClientIP

			// Create top level test object.
			api, err := NewTestWebRTCAPI()
			if err != nil {
				return err
			}
			defer api.Close()

			streamSuffix := fmt.Sprintf("basic-publish-play-%v-%v", os.Getpid(), rand.Int())
			play := NewTestPlayer(api, func(play *TestPlayer) {
				play.streamSuffix = streamSuffix
			})
			defer play.Close()

			pub := NewTestPublisher(api, func(pub *TestPublisher) {
				pub.streamSuffix = streamSuffix
				pub.iceReadyCancel = publishReadyCancel
			})
			defer pub.Close()

			if err := api.Setup(vnetClientIP, func(api *TestWebRTCAPI) {
				var nnWriteRTP, nnReadRTP, nnWriteRTCP, nnReadRTCP int64
				api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
					i.rtpReader = func(buf []byte, attributes interceptor.Attributes) (int, interceptor.Attributes, error) {
						nn, attr, err := i.nextRTPReader.Read(buf, attributes)
						nnReadRTP++
						return nn, attr, err
					}
					i.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
						nn, err := i.nextRTPWriter.Write(header, payload, attributes)

						nnWriteRTP++
						logger.Tf(ctx, "publish rtp=(read:%v write:%v), rtcp=(read:%v write:%v) packets",
							nnReadRTP, nnWriteRTP, nnReadRTCP, nnWriteRTCP)
						return nn, err
					}
				}))
				api.registry.Add(NewRTCPInterceptor(func(i *RTCPInterceptor) {
					i.rtcpReader = func(buf []byte, attributes interceptor.Attributes) (int, interceptor.Attributes, error) {
						nn, attr, err := i.nextRTCPReader.Read(buf, attributes)
						nnReadRTCP++
						return nn, attr, err
					}
					i.rtcpWriter = func(pkts []rtcp.Packet, attributes interceptor.Attributes) (int, error) {
						nn, err := i.nextRTCPWriter.Write(pkts, attributes)
						nnWriteRTCP++
						return nn, err
					}
				}))
			}, func(api *TestWebRTCAPI) {
				var nn uint64
				api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
					i.rtpReader = func(payload []byte, attributes interceptor.Attributes) (int, interceptor.Attributes, error) {
						if nn++; nn >= uint64(playOK) {
							cancel() // Completed.
						}
						logger.Tf(ctx, "play got %v packets", nn)
						return i.nextRTPReader.Read(payload, attributes)
					}
				}))
			}); err != nil {
				return err
			}

			// Set the available objects.
			mainReadyCancel()
			thePublisher = pub
			thePlayer = play

			<-ctx.Done()
			return nil
		}

		if err := doInit(); err != nil {
			r1 = err
		}
	}()

	// Run publisher.
	wg.Add(1)
	go func() {
		defer wg.Done()
		defer cancel()

		select {
		case <-ctx.Done():
			return
		case <-mainReady.Done():
		}

		doPublish := func() error {
			if err := thePublisher.Run(logger.WithContext(ctx), cancel); err != nil {
				return err
			}

			logger.Tf(ctx, "pub done")
			return nil
		}
		if err := doPublish(); err != nil {
			r2 = err
		}
	}()

	// Run player.
	wg.Add(1)
	go func() {
		defer wg.Done()
		defer cancel()

		select {
		case <-ctx.Done():
			return
		case <-mainReady.Done():
		}

		select {
		case <-ctx.Done():
			return
		case <-publishReady.Done():
		}

		doPlay := func() error {
			if err := thePlayer.Run(logger.WithContext(ctx), cancel); err != nil {
				return err
			}

			logger.Tf(ctx, "play done")
			return nil
		}
		if err := doPlay(); err != nil {
			r3 = err
		}

	}()
}

// The srs-server is DTLS server, srs-bench is DTLS client which is active mode.
//     No.1  srs-bench: ClientHello
//     No.2 srs-server: ServerHello, Certificate, ServerKeyExchange, CertificateRequest, ServerHelloDone
//     No.3  srs-bench: Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Encrypted(Finished)
//     No.4 srs-server: ChangeCipherSpec, Encrypted(Finished)
func TestRTCServerDTLSDefault(t *testing.T) {
	if err := filterTestError(func() error {
		ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
		publishOK, vnetClientIP := *srsPublishOKPackets, *srsVnetClientIP

		// Create top level test object.
		api, err := NewTestWebRTCAPI()
		if err != nil {
			return err
		}
		defer api.Close()

		streamSuffix := fmt.Sprintf("dtls-server-no-arq-%v-%v", os.Getpid(), rand.Int())
		p := NewTestPublisher(api, func(p *TestPublisher) {
			p.streamSuffix = streamSuffix
			p.onOffer = testUtilSetupActive
		})
		defer p.Close()

		if err := api.Setup(vnetClientIP, func(api *TestWebRTCAPI) {
			var nn int64
			api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
				i.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
					if nn++; nn >= int64(publishOK) {
						cancel() // Send enough packets, done.
					}
					logger.Tf(ctx, "publish write %v packets", nn)
					return i.nextRTPWriter.Write(header, payload, attributes)
				}
			}))
		}, func(api *TestWebRTCAPI) {
			api.router.AddChunkFilter(func(c vnet.Chunk) (ok bool) {
				chunk, parsed := NewChunkMessageType(c)
				if !parsed {
					return true
				}
				logger.Tf(ctx, "Chunk %v, ok=%v %v bytes", chunk, ok, len(c.UserData()))
				return true
			})
		}); err != nil {
			return err
		}

		return p.Run(ctx, cancel)
	}()); err != nil {
		t.Errorf("err %+v", err)
	}
}

// The srs-server is DTLS client, srs-bench is DTLS server which is passive mode.
//     No.1 srs-server: ClientHello
//     No.2  srs-bench: ServerHello, Certificate, ServerKeyExchange, CertificateRequest, ServerHelloDone
//     No.3 srs-server: Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Encrypted(Finished)
//     No.4  srs-bench: ChangeCipherSpec, Encrypted(Finished)
func TestRTCClientDTLSDefault(t *testing.T) {
	if err := filterTestError(func() error {
		ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
		publishOK, vnetClientIP := *srsPublishOKPackets, *srsVnetClientIP

		// Create top level test object.
		api, err := NewTestWebRTCAPI()
		if err != nil {
			return err
		}
		defer api.Close()

		streamSuffix := fmt.Sprintf("dtls-client-no-arq-%v-%v", os.Getpid(), rand.Int())
		p := NewTestPublisher(api, func(p *TestPublisher) {
			p.streamSuffix = streamSuffix
			p.onOffer = testUtilSetupPassive
		})
		defer p.Close()

		if err := api.Setup(vnetClientIP, func(api *TestWebRTCAPI) {
			var nn int64
			api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
				i.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
					if nn++; nn >= int64(publishOK) {
						cancel() // Send enough packets, done.
					}
					logger.Tf(ctx, "publish write %v packets", nn)
					return i.nextRTPWriter.Write(header, payload, attributes)
				}
			}))
		}, func(api *TestWebRTCAPI) {
			api.router.AddChunkFilter(func(c vnet.Chunk) (ok bool) {
				chunk, parsed := NewChunkMessageType(c)
				if !parsed {
					return true
				}
				logger.Tf(ctx, "Chunk %v, ok=%v %v bytes", chunk, ok, len(c.UserData()))
				return true
			})
		}); err != nil {
			return err
		}

		return p.Run(ctx, cancel)
	}()); err != nil {
		t.Errorf("err %+v", err)
	}
}

// The srs-server is DTLS server, srs-bench is DTLS client which is active mode.
// When srs-bench close the PC, it will send DTLS alert and might retransmit it.
func TestRTCServerDTLSArqAlert(t *testing.T) {
	if err := filterTestError(func() error {
		ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
		publishOK, vnetClientIP := *srsPublishOKPackets, *srsVnetClientIP

		// Create top level test object.
		api, err := NewTestWebRTCAPI()
		if err != nil {
			return err
		}
		defer api.Close()

		streamSuffix := fmt.Sprintf("dtls-client-no-arq-%v-%v", os.Getpid(), rand.Int())
		p := NewTestPublisher(api, func(p *TestPublisher) {
			p.streamSuffix = streamSuffix
			p.onOffer = testUtilSetupActive
		})
		defer p.Close()

		if err := api.Setup(vnetClientIP, func(api *TestWebRTCAPI) {
			// Send enough packets, done.
			var nn int64
			api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
				i.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
					if nn++; nn >= int64(publishOK) {
						cancel()
					}
					logger.Tf(ctx, "publish write %v packets", nn)
					return i.nextRTPWriter.Write(header, payload, attributes)
				}
			}))
		}, func(api *TestWebRTCAPI) {
			api.router.AddChunkFilter(func(c vnet.Chunk) (ok bool) {
				chunk, parsed := NewChunkMessageType(c)
				if !parsed || chunk.chunk != ChunkTypeDTLS {
					return true
				}

				// Copy the alert to server, ignore error.
				if chunk.content == DTLSContentTypeAlert {
					_ = api.proxy.Deliver(c.SourceAddr(), c.DestinationAddr(), c.UserData())
					_ = api.proxy.Deliver(c.SourceAddr(), c.DestinationAddr(), c.UserData())
				}

				logger.Tf(ctx, "Chunk %v, ok=%v %v bytes", chunk, ok, len(c.UserData()))
				return true
			})
		}); err != nil {
			return err
		}

		return p.Run(ctx, cancel)
	}()); err != nil {
		t.Errorf("err %+v", err)
	}
}

// The srs-server is DTLS client, srs-bench is DTLS server which is passive mode.
// When srs-bench close the PC, it will send DTLS alert and might retransmit it.
func TestRTCClientDTLSArqAlert(t *testing.T) {
	if err := filterTestError(func() error {
		ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
		publishOK, vnetClientIP := *srsPublishOKPackets, *srsVnetClientIP

		// Create top level test object.
		api, err := NewTestWebRTCAPI()
		if err != nil {
			return err
		}
		defer api.Close()

		streamSuffix := fmt.Sprintf("dtls-client-no-arq-%v-%v", os.Getpid(), rand.Int())
		p := NewTestPublisher(api, func(p *TestPublisher) {
			p.streamSuffix = streamSuffix
			p.onOffer = testUtilSetupPassive
		})
		defer p.Close()

		if err := api.Setup(vnetClientIP, func(api *TestWebRTCAPI) {
			// Send enough packets, done.
			var nn int64
			api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
				i.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
					if nn++; nn >= int64(publishOK) {
						cancel()
					}
					logger.Tf(ctx, "publish write %v packets", nn)
					return i.nextRTPWriter.Write(header, payload, attributes)
				}
			}))
		}, func(api *TestWebRTCAPI) {
			api.router.AddChunkFilter(func(c vnet.Chunk) (ok bool) {
				chunk, parsed := NewChunkMessageType(c)
				if !parsed || chunk.chunk != ChunkTypeDTLS {
					return true
				}

				// Copy the alert to server, ignore error.
				if chunk.content == DTLSContentTypeAlert {
					_ = api.proxy.Deliver(c.SourceAddr(), c.DestinationAddr(), c.UserData())
					_ = api.proxy.Deliver(c.SourceAddr(), c.DestinationAddr(), c.UserData())
				}

				logger.Tf(ctx, "Chunk %v, ok=%v %v bytes", chunk, ok, len(c.UserData()))
				return true
			})
		}); err != nil {
			return err
		}

		return p.Run(ctx, cancel)
	}()); err != nil {
		t.Errorf("err %+v", err)
	}
}

// The srs-server is DTLS server, srs-bench is DTLS client which is active mode.
//        No.1  srs-bench: ClientHello
// [Drop] No.2 srs-server: ServerHello, Certificate, ServerKeyExchange, CertificateRequest, ServerHelloDone
// [ARQ]  No.3  srs-bench: ClientHello
//        No.4 srs-server: ServerHello, Certificate, ServerKeyExchange, CertificateRequest, ServerHelloDone
//        No.5  srs-bench: Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Encrypted(Finished)
//        No.6 srs-server: ChangeCipherSpec, Encrypted(Finished)
func TestRTCServerDTLSArqServerHello(t *testing.T) {
	if err := filterTestError(func() error {
		ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
		publishOK, vnetClientIP := *srsPublishOKPackets, *srsVnetClientIP

		// Create top level test object.
		api, err := NewTestWebRTCAPI()
		if err != nil {
			return err
		}
		defer api.Close()

		streamSuffix := fmt.Sprintf("dtls-server-arq-server-hello-%v-%v", os.Getpid(), rand.Int())
		p := NewTestPublisher(api, func(p *TestPublisher) {
			p.streamSuffix = streamSuffix
			p.onOffer = testUtilSetupActive
		})
		defer p.Close()

		if err := api.Setup(vnetClientIP, func(api *TestWebRTCAPI) {
			// Send enough packets, done.
			var nn int64
			api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
				i.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
					if nn++; nn >= int64(publishOK) {
						cancel()
					}
					logger.Tf(ctx, "publish write %v packets", nn)
					return i.nextRTPWriter.Write(header, payload, attributes)
				}
			}))
		}, func(api *TestWebRTCAPI) {
			// How many ServerHello we got.
			var nnServerHello int
			// How many packets should we drop.
			const nnMaxDrop = 1

			// Hijack the network packets.
			api.router.AddChunkFilter(func(c vnet.Chunk) (ok bool) {
				chunk, parsed := NewChunkMessageType(c)
				if !parsed || chunk.chunk != ChunkTypeDTLS {
					return true
				}

				if chunk.content != DTLSContentTypeHandshake || chunk.handshake != DTLSHandshakeTypeServerHello {
					ok = true
				} else {
					nnServerHello++
					ok = (nnServerHello > nnMaxDrop)

					// Matched, slow down for test case.
					time.Sleep(10 * time.Millisecond)
				}

				logger.Tf(ctx, "NN=%v, Chunk %v, ok=%v %v bytes", nnServerHello, chunk, ok, len(c.UserData()))
				return
			})
		}); err != nil {
			return err
		}

		return p.Run(ctx, cancel)
	}()); err != nil {
		t.Errorf("err %+v", err)
	}
}

// The srs-server is DTLS server, srs-bench is DTLS client which is active mode.
//        No.1  srs-bench: ClientHello
//        No.2 srs-server: ServerHello, Certificate, ServerKeyExchange, CertificateRequest, ServerHelloDone
//        No.3  srs-bench: Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Encrypted(Finished)
// [Drop] No.4 srs-server: ChangeCipherSpec, Encrypted(Finished)
// [ARQ]  No.5  srs-bench: Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Encrypted(Finished)
//        No.6 srs-server: ChangeCipherSpec, Encrypted(Finished)
func TestRTCServerDTLSArqChangeCipherSpec(t *testing.T) {
	if err := filterTestError(func() error {
		ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
		publishOK, vnetClientIP := *srsPublishOKPackets, *srsVnetClientIP

		// Create top level test object.
		api, err := NewTestWebRTCAPI()
		if err != nil {
			return err
		}
		defer api.Close()

		streamSuffix := fmt.Sprintf("dtls-server-arq-change-cipher-spec-%v-%v", os.Getpid(), rand.Int())
		p := NewTestPublisher(api, func(p *TestPublisher) {
			p.streamSuffix = streamSuffix
			p.onOffer = testUtilSetupActive
		})
		defer p.Close()

		if err := api.Setup(vnetClientIP, func(api *TestWebRTCAPI) {
			// Send enough packets, done.
			var nn int64
			api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
				i.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
					if nn++; nn >= int64(publishOK) {
						cancel()
					}
					logger.Tf(ctx, "publish write %v packets", nn)
					return i.nextRTPWriter.Write(header, payload, attributes)
				}
			}))
		}, func(api *TestWebRTCAPI) {
			// How many ChangeCipherSpec we got.
			var nnChangeCipherSpec int
			// How many packets should we drop.
			const nnMaxDrop = 1

			// Hijack the network packets.
			api.router.AddChunkFilter(func(c vnet.Chunk) (ok bool) {
				chunk, parsed := NewChunkMessageType(c)
				if !parsed || chunk.chunk != ChunkTypeDTLS {
					return true
				}

				if chunk.content != DTLSContentTypeChangeCipherSpec {
					ok = true
				} else {
					nnChangeCipherSpec++
					ok = (nnChangeCipherSpec > nnMaxDrop)

					// Matched, slow down for test case.
					time.Sleep(10 * time.Millisecond)
				}

				logger.Tf(ctx, "NN=%v, Chunk %v, ok=%v %v bytes", nnChangeCipherSpec, chunk, ok, len(c.UserData()))
				return
			})
		}); err != nil {
			return err
		}

		return p.Run(ctx, cancel)
	}()); err != nil {
		t.Errorf("err %+v", err)
	}
}

// The srs-server is DTLS client, srs-bench is DTLS server which is passive mode.
// [Drop] No.1 srs-server: ClientHello
// [ARQ]  No.2 srs-server: ClientHello
//        No.3  srs-bench: ServerHello, Certificate, ServerKeyExchange, CertificateRequest, ServerHelloDone
//        No.4 srs-server: Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Encrypted(Finished)
//        No.5  srs-bench: ChangeCipherSpec, Encrypted(Finished)
func TestRTCClientDTLSArqClientHello_ByDropRequest(t *testing.T) {
	if err := filterTestError(func() error {
		ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
		publishOK, vnetClientIP := *srsPublishOKPackets, *srsVnetClientIP

		// Create top level test object.
		api, err := NewTestWebRTCAPI()
		if err != nil {
			return err
		}
		defer api.Close()

		streamSuffix := fmt.Sprintf("dtls-server-arq-client-hello-%v-%v", os.Getpid(), rand.Int())
		p := NewTestPublisher(api, func(p *TestPublisher) {
			p.streamSuffix = streamSuffix
			p.onOffer = testUtilSetupPassive
		})
		defer p.Close()

		if err := api.Setup(vnetClientIP, func(api *TestWebRTCAPI) {
			// Send enough packets, done.
			var nn int64
			api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
				i.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
					if nn++; nn >= int64(publishOK) {
						cancel()
					}
					logger.Tf(ctx, "publish write %v packets", nn)
					return i.nextRTPWriter.Write(header, payload, attributes)
				}
			}))
		}, func(api *TestWebRTCAPI) {
			// How many ClientHello we got.
			var nnClientHello int
			// How many packets should we drop.
			const nnMaxDrop = 1

			// Hijack the network packets.
			api.router.AddChunkFilter(func(c vnet.Chunk) (ok bool) {
				chunk, parsed := NewChunkMessageType(c)
				if !parsed || chunk.chunk != ChunkTypeDTLS {
					return true
				}

				if chunk.content != DTLSContentTypeHandshake || chunk.handshake != DTLSHandshakeTypeClientHello {
					ok = true
				} else {
					nnClientHello++
					ok = (nnClientHello > nnMaxDrop)

					// Matched, slow down for test case.
					time.Sleep(10 * time.Millisecond)
				}

				logger.Tf(ctx, "NN=%v, Chunk %v, ok=%v %v bytes", nnClientHello, chunk, ok, len(c.UserData()))
				return
			})
		}); err != nil {
			return err
		}

		return p.Run(ctx, cancel)
	}()); err != nil {
		t.Errorf("err %+v", err)
	}
}

// The srs-server is DTLS client, srs-bench is DTLS server which is passive mode.
//        No.1 srs-server: ClientHello
//        No.2  srs-bench: ServerHello, Certificate, ServerKeyExchange, CertificateRequest, ServerHelloDone
// [Drop] No.3 srs-server: Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Encrypted(Finished)
// [ARQ]  No.4 srs-server: Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Encrypted(Finished)
//        No.5  srs-bench: ChangeCipherSpec, Encrypted(Finished)
func TestRTCClientDTLSArqCertificate_ByDropRequest(t *testing.T) {
	if err := filterTestError(func() error {
		ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
		publishOK, vnetClientIP := *srsPublishOKPackets, *srsVnetClientIP

		// Create top level test object.
		api, err := NewTestWebRTCAPI()
		if err != nil {
			return err
		}
		defer api.Close()

		streamSuffix := fmt.Sprintf("dtls-server-arq-certificate-%v-%v", os.Getpid(), rand.Int())
		p := NewTestPublisher(api, func(p *TestPublisher) {
			p.streamSuffix = streamSuffix
			p.onOffer = testUtilSetupPassive
		})
		defer p.Close()

		if err := api.Setup(vnetClientIP, func(api *TestWebRTCAPI) {
			// Send enough packets, done.
			var nn int64
			api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
				i.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
					if nn++; nn >= int64(publishOK) {
						cancel()
					}
					logger.Tf(ctx, "publish write %v packets", nn)
					return i.nextRTPWriter.Write(header, payload, attributes)
				}
			}))
		}, func(api *TestWebRTCAPI) {
			// How many Certificate we got.
			var nnCertificate int
			// How many packets should we drop.
			const nnMaxDrop = 1

			// Hijack the network packets.
			api.router.AddChunkFilter(func(c vnet.Chunk) (ok bool) {
				chunk, parsed := NewChunkMessageType(c)
				if !parsed || chunk.chunk != ChunkTypeDTLS {
					return true
				}

				if chunk.content != DTLSContentTypeHandshake || chunk.handshake != DTLSHandshakeTypeCertificate {
					ok = true
				} else {
					nnCertificate++
					ok = (nnCertificate > nnMaxDrop)

					// Matched, slow down for test case.
					time.Sleep(10 * time.Millisecond)
				}

				logger.Tf(ctx, "NN=%v, Chunk %v, ok=%v %v bytes", nnCertificate, chunk, ok, len(c.UserData()))
				return
			})
		}); err != nil {
			return err
		}

		return p.Run(ctx, cancel)
	}()); err != nil {
		t.Errorf("err %+v", err)
	}
}

// The srs-server is DTLS client, srs-bench is DTLS server which is passive mode.
//        No.1 srs-server: ClientHello
// [Drop] No.2  srs-bench: ServerHello, Certificate, ServerKeyExchange, CertificateRequest, ServerHelloDone
// [ARQ]  No.3 srs-server: ClientHello
//        No.4  srs-bench: ServerHello, Certificate, ServerKeyExchange, CertificateRequest, ServerHelloDone
//        No.5 srs-server: Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Encrypted(Finished)
//        No.6  srs-bench: ChangeCipherSpec, Encrypted(Finished)
func TestRTCClientDTLSArqClientHello_ByDropResponse(t *testing.T) {
	if err := filterTestError(func() error {
		ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
		publishOK, vnetClientIP := *srsPublishOKPackets, *srsVnetClientIP

		// Create top level test object.
		api, err := NewTestWebRTCAPI()
		if err != nil {
			return err
		}
		defer api.Close()

		streamSuffix := fmt.Sprintf("dtls-server-arq-client-hello-%v-%v", os.Getpid(), rand.Int())
		p := NewTestPublisher(api, func(p *TestPublisher) {
			p.streamSuffix = streamSuffix
			p.onOffer = testUtilSetupPassive
		})
		defer p.Close()

		if err := api.Setup(vnetClientIP, func(api *TestWebRTCAPI) {
			// Send enough packets, done.
			var nn int64
			api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
				i.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
					if nn++; nn >= int64(publishOK) {
						cancel()
					}
					logger.Tf(ctx, "publish write %v packets", nn)
					return i.nextRTPWriter.Write(header, payload, attributes)
				}
			}))
		}, func(api *TestWebRTCAPI) {
			// How many ServerHello we got.
			var nnServerHello int
			// How many packets should we drop.
			const nnMaxDrop = 1

			// Hijack the network packets.
			api.router.AddChunkFilter(func(c vnet.Chunk) (ok bool) {
				chunk, parsed := NewChunkMessageType(c)
				if !parsed || chunk.chunk != ChunkTypeDTLS {
					return true
				}

				if chunk.content != DTLSContentTypeHandshake || chunk.handshake != DTLSHandshakeTypeServerHello {
					ok = true
				} else {
					nnServerHello++
					ok = (nnServerHello > nnMaxDrop)

					// Matched, slow down for test case.
					time.Sleep(10 * time.Millisecond)
				}

				logger.Tf(ctx, "NN=%v, Chunk %v, ok=%v %v bytes", nnServerHello, chunk, ok, len(c.UserData()))
				return
			})
		}); err != nil {
			return err
		}

		return p.Run(ctx, cancel)
	}()); err != nil {
		t.Errorf("err %+v", err)
	}
}

// The srs-server is DTLS client, srs-bench is DTLS server which is passive mode.
//        No.1 srs-server: ClientHello
//        No.2  srs-bench: ServerHello, Certificate, ServerKeyExchange, CertificateRequest, ServerHelloDone
//        No.3 srs-server: Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Encrypted(Finished)
// [Drop] No.4  srs-bench: ChangeCipherSpec, Encrypted(Finished)
//        No.5 srs-server: Certificate, ClientKeyExchange, CertificateVerify, ChangeCipherSpec, Encrypted(Finished)
// [ARQ]  No.6  srs-bench: ChangeCipherSpec, Encrypted(Finished)
func TestRTCClientDTLSArqCertificate_ByDropResponse(t *testing.T) {
	if err := filterTestError(func() error {
		ctx, cancel := context.WithTimeout(logger.WithContext(context.Background()), time.Duration(*srsTimeout)*time.Millisecond)
		publishOK, vnetClientIP := *srsPublishOKPackets, *srsVnetClientIP

		// Create top level test object.
		api, err := NewTestWebRTCAPI()
		if err != nil {
			return err
		}
		defer api.Close()

		streamSuffix := fmt.Sprintf("dtls-server-arq-certificate-%v-%v", os.Getpid(), rand.Int())
		p := NewTestPublisher(api, func(p *TestPublisher) {
			p.streamSuffix = streamSuffix
			p.onOffer = testUtilSetupPassive
		})
		defer p.Close()

		if err := api.Setup(vnetClientIP, func(api *TestWebRTCAPI) {
			// Send enough packets, done.
			var nn int64
			api.registry.Add(NewRTPInterceptor(func(i *RTPInterceptor) {
				i.rtpWriter = func(header *rtp.Header, payload []byte, attributes interceptor.Attributes) (int, error) {
					if nn++; nn >= int64(publishOK) {
						cancel()
					}
					logger.Tf(ctx, "publish write %v packets", nn)
					return i.nextRTPWriter.Write(header, payload, attributes)
				}
			}))
		}, func(api *TestWebRTCAPI) {
			// How many ChangeCipherSpec we got.
			var nnChangeCipherSpec int
			// How many packets should we drop.
			const nnMaxDrop = 1

			// Hijack the network packets.
			api.router.AddChunkFilter(func(c vnet.Chunk) (ok bool) {
				chunk, parsed := NewChunkMessageType(c)
				if !parsed || chunk.chunk != ChunkTypeDTLS {
					return true
				}

				if chunk.content != DTLSContentTypeChangeCipherSpec {
					ok = true
				} else {
					nnChangeCipherSpec++
					ok = (nnChangeCipherSpec > nnMaxDrop)

					// Matched, slow down for test case.
					time.Sleep(10 * time.Millisecond)
				}

				logger.Tf(ctx, "NN=%v, Chunk %v, ok=%v %v bytes", nnChangeCipherSpec, chunk, ok, len(c.UserData()))
				return
			})
		}); err != nil {
			return err
		}

		return p.Run(ctx, cancel)
	}()); err != nil {
		t.Errorf("err %+v", err)
	}
}
