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

int StRtmpPublishClient::Dump(RtmpUrl* url){
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

int StRtmpPublishClient::Connect(RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    
    StSocket* socket = (StSocket*)srs_hijack_io_get(srs);
    
    if(socket && socket->Status() == SocketConnected){
        return ret;
    }
    
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

int StRtmpPublishClient::PlayStram(){
    return srs_rtmp_play_stream(srs);
}

int StRtmpPublishClient::DumpAV(){
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
