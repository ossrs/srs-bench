SB(SRS Bench)
===========

hls/http/rtmp-play/rtmp-publish load test tool base on st(state-threads), support huge concurrency<br/>

## About

服务器负载测试工具SB(SRS Bench)：

1. 模拟huge并发：2G内存就可以开300k连接。基于states-threads的协程。
1. 支持HLS解析和测试，下载ts片后等待一个切片长度，模拟客户端。支持HLS点播和直播。执行程序：`./objs/sb_hls_load`
1. 支持HTTP负载测试，所有并发重复下载一个http文件。可将80Gbps带宽测试的72Gbps。执行程序：`./objs/sb_http_load `
1. 支持RTMP流播放测试，一个进程支持5k并发。执行程序：`./objs/sb_rtmp_load`
1. 支持RTMP流推流测试，一个进程支持500个并发。执行程序：`./objs/sb_rtmp_publish`
1. RTMP协议使用高性能服务器SRS([SimpleRtmpServer](https://github.com/ossrs/srs))的协议栈。

注意：

1. HTTP/HLS：依赖服务器Content-Length，不支持chunked方式(chunked时会把所有内容当做body一直读)。
2. 所有程序都在Linux下运行，模拟客户端运行。
3. 其他工具参考[srs-librtmp](https://github.com/ossrs/srs/wiki/v2_CN_SrsLibrtmp#srs-librtmp-examples)

## Usage

```
git clone https://github.com/simple-rtmp-server/srs-bench.git &&
cd srs-bench && ./configure && make &&
./objs/sb_rtmp_load -c 1 -r rtmp://127.0.0.1:1935/live/livestream
```

## Benchmarks

TestEnvironment: 24CPU, 80Gbps Network, 16GB Memory<br/>
Server: NGINX HLS<br/>
Result: 90% bandwith, 72Gbps

<pre>
[root@dell-server ~]# dstat
----total-cpu-usage---- -dsk/total- -net/total- ---paging-- ---system--
usr sys idl wai hiq siq| read  writ| recv  send  |  in   out | int   csw 
  1   1  95   0   0   3|4091B  369k|   0     0   |   0     0 | 100k 9545 
  3   8  66   0   0  23|   0     0 |  40MB 6114MB|   0     0 | 681k   46k
  3   8  63   0   0  25|   0   100k|  41MB 6223MB|   0     0 | 692k   46k
  3   8  64   0   0  25|   0     0 |  41MB 6190MB|   0     0 | 694k   45k
  3   8  66   0   0  23|   0     0 |  40MB 6272MB|   0     0 | 694k   48k
  3   8  64   0   0  25|   0    20k|  40MB 6161MB|   0     0 | 687k   46k
  3   8  65   0   0  24|   0     0 |  40MB 6198MB|   0     0 | 687k   46k
  3   8  66   0   0  23|   0     0 |  40MB 6231MB|   0     0 | 688k   47k
  3   7  70   0   0  20|   0    68k|  40MB 6159MB|   0     0 | 675k   49k
  3   9  62   0   0  26|   0  4096B|  42MB 6283MB|   0     0 | 702k   44k
  4   8  62   0   0  25|   0  2472k|  40MB 6122MB|   0     0 | 698k   44k
  3   8  67   0   0  22|   0     0 |  39MB 6066MB|   0     0 | 671k   46k
  3   8  64   0   0  25|   0     0 |  41MB 6263MB|   0     0 | 695k   46k
  3   8  64   0   0  25|4096B  132k|  41MB 6161MB|   0     0 | 687k   45k
  3  11  60   0   0  26|   0     0 |  42MB 6822MB|   0     0 | 714k   36k
  3  10  62   0   0  25|   0     0 |  40MB 6734MB|   0     0 | 703k   38k
  3  11  60   0   0  26|   0     0 |  43MB 7019MB|   0     0 | 724k   38k
  3  11  60   0   0  26|   0    24k|  45MB 7436MB|   0     0 | 746k   41k
  3  11  60   0   0  27|   0    24k|  47MB 7736MB|   0     0 | 766k   42k
  3  11  59   0   0  28|   0     0 |  52MB 8283MB|   0     0 | 806k   45k
  2  10  61   0   0  27|   0     0 |  54MB 8359MB|   0     0 | 806k   47k
  3  12  53   0   0  32|   0    16k|  58MB 8565MB|   0     0 | 850k   42k
  2  10  62   0   0  26|   0  1212k|  51MB 8140MB|   0     0 | 783k   47k
  2  10  64   0   0  24|   0     0 |  42MB 7033MB|   0     0 | 703k   40k
  2  11  62   0   0  25|   0     0 |  43MB 7203MB|   0     0 | 703k   40k
  2  12  57   0   0  29|   0     0 |  50MB 7970MB|   0     0 | 774k   40k
  2  11  54   0   0  33|   0     0 |  72MB 8943MB|   0     0 | 912k   47k
  3  13  65   0   0  20|   0     0 |  36MB 7247MB|   0     0 | 552k   29k
  3  14  61   0   0  23|   0  1492k|  42MB 8091MB|   0     0 | 613k   32k
  3  13  54   0   0  30|   0     0 |  57MB 9144MB|   0     0 | 760k   34k
  2  10  55   0   0  32|   0    84k|  69MB 9292MB|   0     0 | 861k   38k
  2   9  58   0   0  31|   0    92k|  71MB 9083MB|   0     0 | 860k   39k
  2   9  56   0   0  33|   0     0 |  78MB 9098MB|   0     0 | 914k   39k
  2   8  61   0   0  30|   0     0 |  73MB 8860MB|   0     0 | 876k   39k
</pre>

RTMP load test:<br/>
<pre>
top - 17:57:24 up  7:10,  7 users,  load average: 0.20, 0.20, 0.09
Tasks: 154 total,   1 running, 153 sleeping,   0 stopped,   0 zombie
Cpu(s):  7.4%us,  7.2%sy,  0.0%ni, 78.8%id,  0.0%wa,  0.1%hi,  6.5%si,  0.0%st
Mem:   2055440k total,  1304528k used,   750912k free,   182336k buffers
Swap:  2064376k total,        0k used,  2064376k free,   613848k cached

  PID USER      PR  NI  VIRT  RES  SHR S %CPU %MEM    TIME+  COMMAND
13091 winlin    20   0  186m 110m 1404 S 29.6  5.5   1:55.35 ./objs/sb_rtmp_load -c 1000 
12544 winlin    20   0  124m  22m 2080 S 20.3  1.1   1:51.51 ./objs/srs

----total-cpu-usage---- -dsk/total- ---net/lo-- ---paging-- ---system--
usr sys idl wai hiq siq| read  writ| recv  send|  in   out | int   csw 
  7   7  82   0   0   4|   0     0 | 158M  158M|   0     0 |2962   353 
  6   5  83   0   0   6|   0     0 |  74M   74M|   0     0 |2849   291 
  7   6  81   0   0   6|   0     0 | 102M  102M|   0     0 |2966   360 
  7   8  79   0   0   6|   0     0 | 168M  168M|   0     0 |2889   321 
  7   7  79   0   0   7|   0     0 |  83M   83M|   0     0 |2862   364 
  5   6  83   0   0   6|   0     0 | 106M  106M|   0     0 |2967   296 
  5   6  83   0   0   6|   0     0 |  54M   54M|   0     0 |2907   355 
  6   6  84   0   0   4|   0     0 |  58M   58M|   0     0 |2986   353 
  6   6  83   0   0   4|   0     0 | 117M  117M|   0     0 |2863   326 
  7   6  82   0   0   5|   0     0 |  97M   97M|   0     0 |2954   321 
  5   7  78   2   0   8|   0    40k|  82M   82M|   0     0 |2909   357 
  5   5  84   0   0   6|   0     0 |  57M   57M|   0     0 |2937   307 
  8   8  78   0   0   6|   0     0 | 190M  190M|   0     0 |3024   413 
  5   7  82   0   0   7|   0     0 |  75M   75M|   0     0 |2940   310 
  8   8  80   0   0   4|   0     0 | 136M  136M|   0     0 |3000   436 
  8   8  74   0   0  10|   0     0 | 116M  116M|   0     0 |2816   356 
  7   8  78   0   0   6|   0     0 | 128M  128M|   0     0 |2972   424 
  6   8  80   0   0   7|   0  4096B| 123M  123M|   0     0 |2981   395 
  6   6  83   0   0   5|   0     0 |  50M   50M|   0     0 |2984   367 
  7   6  81   2   0   4|   0    92k|  49M   49M|   0     0 |3010   445 
  5   6  84   0   0   6|   0     0 |  22M   22M|   0     0 |2912   364 
  5   5  85   0   0   4|   0     0 |  34M   34M|   0     0 |3001   429 
  6   6  81   0   0   7|   0     0 |  45M   45M|   0     0 |2996   468 
  5   5  84   0   0   6|   0     0 |  18M   18M|   0     0 |2923   338 
  8   8  77   0   0   7|   0     0 | 158M  158M|   0     0 |2971   351 
  7   7  80   0   0   5|   0     0 | 167M  167M|   0     0 |2860   334 
  6   5  83   0   0   6|   0    60k|  61M   61M|   0     0 |2988   424 
  7   8  79   0   0   6|   0     0 | 140M  140M|   0     0 |2916   391 
  8   8  78   0   0   6|   0     0 | 172M  172M|   0     0 |2961   348 
  7   8  78   0   0   7|   0     0 | 127M  127M|   0     0 |2865   347 
  5   6  84   0   0   5|   0     0 |  73M   73M|   0     0 |2972   344 
  6   8  78   0   0   8|   0     0 | 115M  115M|   0     0 |2942   314 
  7   8  79   0   0   6|   0     0 | 147M  147M|   0     0 |2966   366
</pre>

Winlin 2014.12
