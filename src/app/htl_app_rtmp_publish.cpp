/*
The MIT License (MIT)

Copyright (c) 2013-2014 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <assert.h>

#include <string>
#include <sstream>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>

#include <htl_app_rtmp_publish.hpp>
#include <htl_app_rtmp_protocol.hpp>

StRtmpPublishClient::StRtmpPublishClient(){
    stream_id = 0;
    srs = NULL;
}

StRtmpPublishClient::~StRtmpPublishClient(){
    srs_rtmp_destroy(srs);
}

int StRtmpPublishClient::Publish(string input, RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    
    if((ret = Connect(url)) != ERROR_SUCCESS){
        Error("rtmp client connect server failed. ret=%d", ret);
        return ret;
    }
    
    if((ret = Handshake()) != ERROR_SUCCESS){
        Error("rtmp client handshake failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client handshake success");
    
    if((ret = ConnectApp()) != ERROR_SUCCESS){
        Error("rtmp client connect tcUrl failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client connect tcUrl(%s) success", url->GetTcUrl());
    
    if((ret = PublishStram()) != ERROR_SUCCESS){
        Error("rtmp client publish stream failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client publish stream(%s) success", url->GetUrl());
    
    srs_flv_t flv = srs_flv_open_read(input.c_str());
    if (!flv) {
        ret = ERROR_RTMP_OPEN_FLV;
        Error("open flv file %s failed. ret=%d", input.c_str(), ret);
        return ret;
    }

    ret = PublishAV(flv);
    srs_flv_close(flv);
    
    if(ret != ERROR_SUCCESS){
        Error("rtmp client dump av failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client dump av success");

    return ret;
}

int StRtmpPublishClient::Connect(RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    
    srs_rtmp_destroy(srs);
    srs = srs_rtmp_create(url->GetUrl());

    if ((ret = __srs_rtmp_dns_resolve(srs)) != ERROR_SUCCESS){
        Error("dns resolve failed. ret=%d", ret);
        return ret;
    }
    
    if ((ret = __srs_rtmp_connect_server(srs)) != ERROR_SUCCESS){
        Error("connect to server failed. ret=%d", ret);
        return ret;
    }
    
    Info("socket connected on url %s", url->GetUrl());
    
    return ret;
}

int StRtmpPublishClient::Handshake(){
    return __srs_rtmp_do_simple_handshake(srs);
}

int StRtmpPublishClient::ConnectApp(){
    return srs_rtmp_connect_app(srs);
}

int StRtmpPublishClient::PublishStram(){
    return srs_rtmp_publish_stream(srs);
}

int StRtmpPublishClient::PublishAV(srs_flv_t flv){
    int ret = ERROR_SUCCESS;
    
    char header[9];
    if ((ret = srs_flv_read_header(flv, header)) != ERROR_SUCCESS) {
        Error("read flv header failed. ret=%d", ret);
        return ret;
    }

    // open flv and publish to server.
    u_int32_t re = 0;
    while(true){
        char type;
        u_int32_t timestamp;
        int32_t size;
        
        if ((ret = srs_flv_read_tag_header(flv, &type, &size, &timestamp)) != ERROR_SUCCESS) {
            return ret;
        }
        
        char* data = new char[size];
        if ((ret = srs_flv_read_tag_data(flv, data, size)) != ERROR_SUCCESS) {
            delete data;
            return ret;
        }
        
        if ((ret = srs_rtmp_write_packet(srs, type, timestamp, data, size)) != ERROR_SUCCESS) {
            return ret;
        }
        
        Trace("send message type=%d, size=%d, time=%d", type, size, timestamp);
        
        if (re <= 0) {
            re = timestamp;
        }
        
        if (timestamp - re > 300) {
            st_usleep((timestamp - re) * 1000);
            re = timestamp;
        }
    }
    
    return ret;
}
