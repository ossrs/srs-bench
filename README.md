# srs-bench

WebRTC benchmark on [pion/webrtc](https://github.com/pion/webrtc) for [SRS](https://github.com/ossrs/srs).

## Usage

编译和使用：

```bash
git clone https://github.com/ossrs/srs-bench.git && git checkout feature/rtc && 
make && ./objs/srs_bench -h
```

## Player for Live

直播播放压测，一个流，很多个播放。

首先，推流到SRS：

```bash
ffmpeg -re -i doc/source.200kbps.768x320.flv -c copy -f flv -y rtmp://localhost/live/livestream
```

然后，启动压测，比如100个：

```bash
./objs/srs_bench -sr webrtc://localhost/live/livestream -nn 100
```

## Publisher for Live or RTC

直播或会议场景推流压测，一般会推多个流。

首先，推流依赖于录制的文件，请参考[DVR](#dvr)。

然后，启动推流压测，比如100个流：

```bash
./objs/srs_bench -pr webrtc://localhost/live/livestream_%d -sn 100 -sa a.ogg -sv v.h264 -fps 25
```

> 注意：帧率是原始视频的帧率，由于264中没有这个信息所以需要传递。

## Multipel Player or Publisher for RTC

会议场景的播放压测，会多个客户端播放多个流，比如3人会议，那么就有3个推流，每个流有2个播放。

首先，启动推流压测，比如3个流：

```bash
./objs/srs_bench -pr webrtc://localhost/live/livestream_%d -sn 3 -sa a.ogg -sv v.h264 -fps 25
```

然后，每个流都启动播放压测，比如每个流2个播放：

```bash
./objs/srs_bench -sr webrtc://localhost/live/livestream_%d -sn 3 -nn 2
```

> 备注：压测都是基于流，可以任意设计推流和播放的流路数，实现不同的场景。

> 备注：URL的变量格式参考Go的`fmt.Sprintf`，比如可以用`webrtc://localhost/live/livestream_%03d`。

## DVR

录制场景，主要是把内容录制下来后，可分析，也可以用于推流。

首先，推流到SRS，参考[live](#player-for-live)。

```bash
ffmpeg -re -i doc/source.200kbps.768x320.flv -c copy -f flv -y rtmp://localhost/live/livestream
```

然后，启动录制：

```bash
./objs/srs_bench -sr webrtc://localhost/live/livestream -da a.ogg -dv v.h264
```

> 备注：录制下来的`a.ogg`和`v.h264`可以用做推流。

## RTC Plaintext

压测RTC明文播放：

首先，推流到SRS：

```bash
ffmpeg -re -i doc/source.200kbps.768x320.flv -c copy -f flv -y rtmp://localhost/live/livestream
```

然后，启动压测，指定是明文（非加密），比如100个：

```bash
./objs/srs_bench -sr webrtc://localhost/live/livestream?encrypt=false -nn 100
```

> Note: 可以传递更多参数，详细参考SRS支持的参数。

## 回归测试

支持回归测试，使用方法：

```bash
go test ./srs
```

可以给回归测试传参数，比如：

```bash
go test ./srs -rtc-server=127.0.0.1
```

支持的参数如下：

* `-rtc-server`，设置RTC服务器地址，默认是本机即`127.0.0.1`。

2021.01, Winlin
