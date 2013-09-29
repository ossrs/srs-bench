#!/bin/bash
cpu_id=`ps aux|grep st|grep rtmp|grep load|wc -l`

./objs/st_rtmp_load --clients=3000 --url=rtmp://127.0.0.1:1935/live/livestream id=${cpu_id} 1>/dev/null 2>>report.log &
ret=$?; if [[ 0 -ne ${ret} ]];then echo "start process failed, ret=${ret}"; exit ${ret}; fi

pid=`ps aux|grep st|grep rtmp|grep load|grep id=${cpu_id}|awk '{print $2}'`
ret=$?; if [[ 0 -ne ${ret} ]];then echo "get process pid failed, ret=${ret}"; exit ${ret}; fi

echo "pid=${pid} cpu_id=$cpu_id"

sudo taskset -pc ${cpu_id} ${pid}
ret=$?; if [[ 0 -ne ${ret} ]];then echo "bind cpu failed, ret=${ret}"; exit ${ret}; fi

echo "$pid started bind to ${cpu_id}"
