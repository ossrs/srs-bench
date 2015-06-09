/*
The MIT License (MIT)

Copyright (c) 2013-2015 winlin

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

#include <htl_app_rtmp_play.hpp>
#include <htl_app_rtmp_protocol.hpp>
#include <htl_app_srs_hijack.hpp>

#define SOCK_READ_BUFFER 4096
#define SOCK_READV_NB 1024
#define SOCK_MERGED_READ_MS 800

StRtmpPlayClient::StRtmpPlayClient(){
    stream_id = 0;
    srs = NULL;
}

StRtmpPlayClient::~StRtmpPlayClient(){
    srs_rtmp_destroy(srs);
}

int StRtmpPlayClient::Dump(RtmpUrl* url){
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
    
    if((ret = PlayStram()) != ERROR_SUCCESS){
        Error("rtmp client play stream failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client play stream(%s) success", url->GetUrl());
    
    if((ret = DumpAV()) != ERROR_SUCCESS){
        Error("rtmp client dump av failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client dump av success");

    return ret;
}

int StRtmpPlayClient::Connect(RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    
    srs_rtmp_destroy(srs);
    srs = srs_rtmp_create(url->GetUrl());

    if ((ret = srs_rtmp_dns_resolve(srs)) != ERROR_SUCCESS){
        Error("dns resolve failed. ret=%d", ret);
        return ret;
    }
    
    if ((ret = srs_rtmp_connect_server(srs)) != ERROR_SUCCESS){
        Error("connect to server failed. ret=%d", ret);
        return ret;
    }
    
    Info("socket connected on url %s", url->GetUrl());
    
    return ret;
}

int StRtmpPlayClient::Handshake(){
    return srs_rtmp_do_simple_handshake(srs);
}

int StRtmpPlayClient::ConnectApp(){
    return srs_rtmp_connect_app(srs);
}

int StRtmpPlayClient::PlayStram(){
    return srs_rtmp_play_stream(srs);
}

int StRtmpPlayClient::DumpAV(){
    int ret = ERROR_SUCCESS;

    // recv response
    while(true){
        char type;
        u_int32_t timestamp;
        char* data;
        int size;
        
        if ((ret = srs_rtmp_read_packet(srs, &type, &timestamp, &data, &size)) != ERROR_SUCCESS){
            return ret;
        }
        
        Info("get message type=%d, size=%d", type, size);
        delete data;
    }
    
    return ret;
}

StRtmpPlayClientFast::StRtmpPlayClientFast(){
}

StRtmpPlayClientFast::~StRtmpPlayClientFast(){
}

int StRtmpPlayClientFast::DumpAV(){
    int ret = ERROR_SUCCESS;

    StSocket* skt = srs_hijack_get(srs);
    
    // use buffered iovs to read tcp data.
    char buf[SOCK_READ_BUFFER];
    iovec iovs[SOCK_READV_NB];
    
    for (int i = 0; i < SOCK_READV_NB; i++) {
        iovec& iov = iovs[i];
        iov.iov_base = buf;
        iov.iov_len = SOCK_READ_BUFFER;
    }
    
    // recv response
    while(true){
        ssize_t size = 0;
        if ((ret = skt->Readv(iovs, SOCK_READV_NB, &size)) != ERROR_SUCCESS) {
            return ret;
        }
        
        Info("get message size=%d", size);

        // merged read.
        st_usleep(SOCK_MERGED_READ_MS * 1000);
    }
    
    return ret;
}
