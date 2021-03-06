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
	vnet2 "github.com/ossrs/srs-bench/vnet"
	"github.com/pion/logging"
	"github.com/pion/transport/vnet"
)

func ExampleNewProxy() {
	var clientNetwork *vnet.Net
	var proxy *vnet2.UDPProxy

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
			StaticIP: "27.1.2.3",
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
		address := "192.168.1.10:8000" // The real UDP server address.
		proxy, err = vnet2.NewProxy("udp4", address)
		if err != nil {
			// handle error
		}

		// Start the proxy.
		if err := proxy.Start(router); err != nil {
			// handle error
		}
		defer proxy.Stop()
	}

	// Now, all packets from client, will be proxy to real server, vice versa.
	skt, _ := clientNetwork.ListenPacket("udp4", "27.1.2.3")
	skt.WriteTo([]byte("Hello"), proxy.RealServerAddr())
}
