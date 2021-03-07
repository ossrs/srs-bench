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
package vnet_test

import (
	vnet_proxy "github.com/ossrs/srs-bench/vnet"
	"github.com/pion/logging"
	"github.com/pion/transport/vnet"
	"net"
)

// Proxy one vnet endpoint to one real server endpoint.
// For example:
//		vnet(10.0.0.11:5787) => proxy0 => 192.168.1.10:8000
//		vnet(10.0.0.11:5788) => proxy1 => 192.168.1.10:8001
//		vnet(10.0.0.11:5789) => proxy2 => 192.168.1.10:8002
func ExampleProxyOneToOne() {
	var clientNetwork *vnet.Net
	var proxy0Addr, proxy1Addr, proxy2Addr net.Addr

	// Setup the network and proxy.
	if true {
		// Create vnet WAN with one endpoint, please read from
		// https://github.com/pion/transport/tree/master/vnet#example-wan-with-one-endpoint-vnet
		router, err := vnet.NewRouter(&vnet.RouterConfig{
			CIDR:          "0.0.0.0/0",
			LoggerFactory: logging.NewDefaultLoggerFactory(),
		})
		if err != nil {
			// handle error
		}

		// Create a network and add to router, for example, for client.
		clientNetwork = vnet.NewNet(&vnet.NetConfig{
			StaticIP: "10.0.0.11",
		})
		if err = router.AddNet(clientNetwork); err != nil {
			// handle error
		}

		// Start the router.
		if err := router.Start(); err != nil {
			// handle error
		}
		defer router.Stop()

		// Now, we got a real UDP server to communicate with, we must create a vnet.Net
		// and proxy it to router.
		proxy0, err := vnet_proxy.NewProxy("udp4", "192.168.1.10:8000")
		if err != nil {
			// handle error
		}
		proxy0Addr = proxy0.RealServerAddr()

		// Start the proxy.
		if err := proxy0.Start(router); err != nil {
			// handle error
		}
		defer proxy0.Stop()

		// Create more proxies, note that we should handle error.
		proxy1, err := vnet_proxy.NewProxy("udp4", "192.168.1.10:8001")
		proxy1Addr = proxy1.RealServerAddr()
		proxy1.Start(router)
		defer proxy1.Stop()

		proxy2, err := vnet_proxy.NewProxy("udp4", "192.168.1.10:8002")
		proxy2Addr = proxy2.RealServerAddr()
		proxy2.Start(router)
		defer proxy2.Stop()
	}

	// Now, all packets from client, will be proxy to real server, vice versa.
	skt0, _ := clientNetwork.ListenPacket("udp4", "10.0.0.11:5787")
	skt0.WriteTo([]byte("Hello"), proxy0Addr)

	skt1, _ := clientNetwork.ListenPacket("udp4", "10.0.0.11:5788")
	skt1.WriteTo([]byte("Hello"), proxy1Addr)

	skt2, _ := clientNetwork.ListenPacket("udp4", "10.0.0.11:5789")
	skt2.WriteTo([]byte("Hello"), proxy2Addr)
}

// Proxy many vnet endpoint to one real server endpoint.
// For example:
//		vnet(10.0.0.11:5787) => proxy => 192.168.1.10:8000
//		vnet(10.0.0.11:5788) => proxy => 192.168.1.10:8000
//		vnet(10.0.0.11:5789) => proxy => 192.168.1.10:8000
func ExampleProxyManyToOne() {
	var clientNetwork *vnet.Net
	var proxyAddr net.Addr

	// Setup the network and proxy.
	if true {
		// Create vnet WAN with one endpoint, please read from
		// https://github.com/pion/transport/tree/master/vnet#example-wan-with-one-endpoint-vnet
		router, err := vnet.NewRouter(&vnet.RouterConfig{
			CIDR:          "0.0.0.0/0",
			LoggerFactory: logging.NewDefaultLoggerFactory(),
		})
		if err != nil {
			// handle error
		}

		// Create a network and add to router, for example, for client.
		clientNetwork = vnet.NewNet(&vnet.NetConfig{
			StaticIP: "10.0.0.11",
		})
		if err = router.AddNet(clientNetwork); err != nil {
			// handle error
		}

		// Start the router.
		if err := router.Start(); err != nil {
			// handle error
		}
		defer router.Stop()

		// Now, we got a real UDP server to communicate with, we must create a vnet.Net
		// and proxy it to router.
		proxy, err := vnet_proxy.NewProxy("udp4", "192.168.1.10:8000")
		if err != nil {
			// handle error
		}
		proxyAddr = proxy.RealServerAddr()

		// Start the proxy.
		if err := proxy.Start(router); err != nil {
			// handle error
		}
		defer proxy.Stop()
	}

	// Now, all packets from client, will be proxy to real server, vice versa.
	skt0, _ := clientNetwork.ListenPacket("udp4", "10.0.0.11:5787")
	skt0.WriteTo([]byte("Hello"), proxyAddr)

	skt1, _ := clientNetwork.ListenPacket("udp4", "10.0.0.11:5788")
	skt1.WriteTo([]byte("Hello"), proxyAddr)

	skt2, _ := clientNetwork.ListenPacket("udp4", "10.0.0.11:5789")
	skt2.WriteTo([]byte("Hello"), proxyAddr)
}
