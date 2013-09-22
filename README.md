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

