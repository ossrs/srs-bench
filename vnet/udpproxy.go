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

// A UDP proxy between real server(net.UDPConn) and vnet.UDPConn.
//
// High level design:
//                           ..............................................
//                           :         Virtual Network (vnet)             :
//                           :                                            :
//   +-------+ *         1 +----+         +--------+                      :
//   | :App  |------------>|:Net|--o<-----|:Router |          .............................
//   +-------+             +----+         |        |          :        UDPProxy           :
//                           :            |        |       +----+     +---------+     +---------+     +--------+
//                           :            |        |--->o--|:Net|-->o-| vnet.   |-->o-|  net.   |--->-| :Real  |
//                           :            |        |       +----+     | UDPConn |     | UDPConn |     | Server |
//                           :            |        |          :       +---------+     +---------+     +--------+
//                           :            |        |          ............................:
//                           :            +--------+                       :
//                           ...............................................
//
// The whole big picture:
//                           ......................................
//                           :         Virtual Network (vnet)     :
//                           :                                    :
//   +-------+ *         1 +----+         +--------+              :
//   | :App  |------------>|:Net|--o<-----|:Router |          .............................
//   +-------+             +----+         |        |          :        UDPProxy           :
//   +-----------+ *     1 +----+         |        |       +----+     +---------+     +---------+     +--------+
//   |:STUNServer|-------->|:Net|--o<-----|        |--->o--|:Net|-->o-| vnet.   |-->o-|  net.   |--->-| :Real  |
//   +-----------+         +----+         |        |       +----+     | UDPConn |     | UDPConn |     | Server |
//   +-----------+ *     1 +----+         |        |          :       +---------+     +---------+     +--------+
//   |:TURNServer|-------->|:Net|--o<-----|        |          ............................:
//   +-----------+         +----+ [1]     |        |              :
//                           :          1 |        | 1  <<has>>   :
//                           :      +---<>|        |<>----+ [2]   :
//                           :      |     +--------+      |       :
//                         To form  |      *|             v 0..1  :
//                   a subnet tree  |       o [3]      +-----+    :
//                           :      |       ^          |:NAT |    :
//                           :      |       |          +-----+    :
//                           :      +-------+                     :
//                           ......................................
type UDPProxy struct {
	// The real server address to proxy to.
	realServerAddr *net.UDPAddr
	// Connect to vnet connection, which proxy with.
	vnetSocket vnet.UDPPacketConn

	// Each vnet source, bind to a real socket to server.
	// key is vnet client addr, which is net.Addr
	// value is *net.UDPConn
	endpoints sync.Map

	// For state sync.
	wg             sync.WaitGroup
	disposed       context.Context
	disposedCancel context.CancelFunc
	starting       bool
	started        bool
}

// The network and address of real server.
func NewProxy(network, address string) (*UDPProxy, error) {
	addr, err := net.ResolveUDPAddr(network, address)
	if err != nil {
		return nil, err
	}

	v := &UDPProxy{}
	if actual, ok := proxyMux.LoadOrStore(addr.String(), v); ok {
		v = actual.(*UDPProxy) // Reuse the same proxy.
		return v, nil
	}

	v.realServerAddr = addr
	v.disposed, v.disposedCancel = context.WithCancel(context.Background())

	return v, nil
}

func (v *UDPProxy) RealServerAddr() net.Addr {
	return v.realServerAddr
}

func (v *UDPProxy) Start(router *vnet.Router) error {
	if v.starting || v.started {
		return nil
	}
	v.starting = true

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

	// Got new vnet client, start a new endpoint.
	findEndpointBy := func(addr net.Addr) (*net.UDPConn, error) {
		// Exists binding.
		if value, ok := v.endpoints.Load(addr.String()); ok {
			// Exists endpoint, reuse it.
			return value.(*net.UDPConn), nil
		}

		// Got new vnet client, create new endpoint.
		realSocket, err := net.DialUDP("udp4", nil, v.realServerAddr)
		if err != nil {
			return nil, nil // Drop packet.
		}

		// Bind address.
		v.endpoints.Store(addr.String(), realSocket)

		// Got packet from real server, we should proxy it to vnet.
		v.wg.Add(1)
		go func(vnetClientAddr net.Addr) {
			defer v.wg.Done()

			buf := make([]byte, 1500)
			for v.disposed.Err() == nil {
				n, addr, err := realSocket.ReadFrom(buf)
				if err != nil {
					return
				}

				if n <= 0 || addr == nil {
					continue
				}

				//fmt.Println(fmt.Sprintf("Proxy %v=>%v=>%v %v byte",
				//	addr, realSocket.LocalAddr(), vnetClientAddr, n))
				if _, err := v.vnetSocket.WriteTo(buf[:n], vnetClientAddr); err != nil {
					return
				}
			}
		}(addr)

		return realSocket, nil
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

			if n <= 0 || addr == nil {
				continue
			}

			realSocket, err := findEndpointBy(addr)
			if err != nil {
				continue // Drop packet.
			}

			//fmt.Println(fmt.Sprintf("Proxy %v=>%v=>%v %v byte",
			//	addr, realSocket.LocalAddr(), realSocket.RemoteAddr(), n))
			if _, err := realSocket.Write(buf[:n]); err != nil {
				return
			}
		}
	}()

	v.started = true
	return nil
}

func (v *UDPProxy) Stop() error {
	v.disposedCancel()

	v.endpoints.Range(func(key, value interface{}) bool {
		if realSocket, ok := value.(*net.UDPConn); ok {
			realSocket.Close()
		}
		return true
	})

	if v.vnetSocket != nil {
		v.vnetSocket.Close()
	}

	if v.starting || v.started {
		v.wg.Wait()
	}
	return nil
}

// To support many-to-one mux, for example:
//		vnet(10.0.0.11:5787) => proxy => 192.168.1.10:8000
//		vnet(10.0.0.11:5788) => proxy => 192.168.1.10:8000
//		vnet(10.0.0.11:5789) => proxy => 192.168.1.10:8000
//
// key is string, the UDPProxy.realServerAddr.String()
// value is *UDPProxy
var proxyMux sync.Map
