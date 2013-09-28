#!/bin/bash
cpu_id=`ps aux|grep st|grep hls|grep load|wc -l`

./objs/st_hls_load --thread=3000 --url=http://127.0.0.1:3080/hls/hls.m3u8 --vod id=${cpu_id} 1>/dev/null 2>>report.log &
ret=$?; if [[ 0 -ne ${ret} ]];then echo "start process failed, ret=${ret}"; exit ${ret}; fi

pid=`ps aux|grep st|grep hls|grep load|grep id=${cpu_id}|awk '{print $2}'`
ret=$?; if [[ 0 -ne ${ret} ]];then echo "get process pid failed, ret=${ret}"; exit ${ret}; fi

echo "pid=${pid} cpu_id=$cpu_id"

sudo taskset -pc ${cpu_id} ${pid}
ret=$?; if [[ 0 -ne ${ret} ]];then echo "bind cpu failed, ret=${ret}"; exit ${ret}; fi

echo "$pid started bind to ${cpu_id}"
