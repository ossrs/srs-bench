st-hls-load
===========

hls/http load test tool base on st(state-threads), support huge concurrency

TestEnvironment: 24CPU, 80Gbps Network, 16GB Memory
Server: NGINX HLS

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

