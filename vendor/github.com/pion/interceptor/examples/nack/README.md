# nack
nack demonstrates how to send RTP packets over a connection that both generates and handles NACKs

## Instructions
### run main.go
```
go run main.go
```

You will then see output like

```
$ go run main.go
2020/12/16 00:27:54 Received RTP
2020/12/16 00:27:54 Received RTP
2020/12/16 00:27:54 Received RTP
2020/12/16 00:27:54 Received RTP
2020/12/16 00:27:54 Received RTP
2020/12/16 00:27:55 Received RTP
2020/12/16 00:27:55 Received NACK
2020/12/16 00:27:55 Received RTP
2020/12/16 00:27:55 Received RTP
2020/12/16 00:27:55 Received RTP
2020/12/16 00:27:55 Received RTP
2020/12/16 00:27:56 Received RTP
2020/12/16 00:27:56 Received NACK
2020/12/16 00:27:56 Received RTP
2020/12/16 00:27:56 Received NACK
2020/12/16 00:27:56 Received RTP
2020/12/16 00:27:56 Received RTP
2020/12/16 00:27:56 Received RTP
2020/12/16 00:27:57 Received RTP
2020/12/16 00:27:57 Received RTP
2020/12/16 00:27:57 Received RTP
2020/12/16 00:27:58 Received RTP
2020/12/16 00:27:58 Received NACK
2020/12/16 00:27:58 Received RTP
2020/12/16 00:27:58 Received RTP
2020/12/16 00:27:58 Received RTP
2020/12/16 00:27:58 Received NACK
2020/12/16 00:27:58 Received RTP
2020/12/16 00:27:58 Received RTP
```

### Introduce loss
You will not see much loss on loopback by default. To introduce 15% loss you can do

```
$ iptables -A INPUT -m statistic --mode random --probability 0.15 -p udp -j DROP
```
