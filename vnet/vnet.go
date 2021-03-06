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
package vnet

import (
	"context"
	"github.com/pion/transport/vnet"
	"net"
	"sync"
)

// A UDP proxy between real server(net.UDPConn) and vnet.UDPConn
type UDPProxy struct {
	// Connect to real server.
	realServerAddr *net.UDPAddr
	realSocket     *net.UDPConn

	// Connect to vnet connection.
	vnetClientAddr net.Addr
	vnetSocket     vnet.UDPPacketConn

	// For state sync.
	wg             sync.WaitGroup
	disposed       context.Context
	disposedCancel context.CancelFunc
}

// The network and address of real server.
func NewProxy(network, address string) (*UDPProxy, error) {
	v := &UDPProxy{}

	addr, err := net.ResolveUDPAddr(network, address)
	if err != nil {
		return nil, err
	}

	v.realServerAddr = addr
	v.disposed, v.disposedCancel = context.WithCancel(context.Background())

	return v, nil
}

func (v *UDPProxy) RealServerAddr() net.Addr {
	return v.realServerAddr
}

func (v *UDPProxy) Start(router *vnet.Router) error {
	// Create vnet for real server.
	nw := vnet.NewNet(&vnet.NetConfig{
		StaticIP: v.realServerAddr.IP.String(),
	})
	if err := router.AddNet(nw); err != nil {
		return err
	}

	// We must create a "same" vnet.UDPConn as the net.UDPConn,
	// which has the same ip:port, to copy packets between them.
	var err error
	if v.vnetSocket, err = nw.ListenUDP("udp4", v.realServerAddr); err != nil {
		return err
	}

	// Create a real UDP connection to proxy with vnet packets.
	if v.realSocket, err = net.DialUDP("udp4", nil, v.realServerAddr); err != nil {
		return err
	}

	// Got packet from client, we should proxy it to real server.
	v.wg.Add(1)
	go func() {
		defer v.wg.Done()

		buf := make([]byte, 1500)

		for v.disposed.Err() == nil {
			n, addr, err := v.vnetSocket.ReadFrom(buf)
			if err != nil {
				return
			}

			if n <= 0 {
				continue
			}

			v.vnetClientAddr = addr

			if _, err := v.realSocket.Write(buf[:n]); err != nil {
				return
			}
		}
	}()

	// Got packet from real server, we should proxy it to vnet.
	v.wg.Add(1)
	go func() {
		defer v.wg.Done()

		buf := make([]byte, 1500)

		for v.disposed.Err() == nil {
			n, _, err := v.realSocket.ReadFrom(buf)
			if err != nil {
				return
			}

			if n <= 0 || v.vnetClientAddr == nil {
				continue
			}

			if _, err := v.vnetSocket.WriteTo(buf[:n], v.vnetClientAddr); err != nil {
				return
			}
		}
	}()

	return nil
}

func (v *UDPProxy) Stop() error {
	v.disposedCancel()

	if v.realSocket != nil {
		v.realSocket.Close()
	}
	if v.vnetSocket != nil {
		v.vnetSocket.Close()
	}

	v.wg.Wait()
	return nil
}
