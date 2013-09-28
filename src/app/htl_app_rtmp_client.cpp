#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <assert.h>

#include <string>
#include <sstream>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>

#include <htl_app_rtmp_client.hpp>

StRtmpClient::StRtmpClient(){
    socket = new StSocket();
}

StRtmpClient::~StRtmpClient(){
    delete socket;
    socket = NULL;
}

int StRtmpClient::Dump(RtmpUrl* url){
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
    
    if((ret = ConnectApp(url)) != ERROR_SUCCESS){
        Error("rtmp client connect tcUrl failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client connect tcUrl(%s) success", url->GetTcUrl());
    
    if((ret = PlayStram(url)) != ERROR_SUCCESS){
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

int StRtmpClient::Connect(RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    
    if(socket->Status() == SocketConnected){
        return ret;
    }

    string ip;
    if((ret = StUtility::DnsResolve(url->GetHost(), ip)) != ERROR_SUCCESS){
        Error("dns resolve failed. ret=%d", ret);
        return ret;
    }
    
    if((ret = socket->Connect(ip.c_str(), url->GetPort())) != ERROR_SUCCESS){
        Error("connect to server failed. ret=%d", ret);
        return ret;
    }
    
    Info("socket connected on url %s", url->GetUrl());
    
    return ret;
}

int StRtmpClient::Handshake(){
    int ret = ERROR_SUCCESS;
    
    ssize_t nsize;
    
    char buf[1537];
    buf[0] = 0x03; // plain text.

    char* c0c1 = buf;
    if((ret = socket->Write(c0c1, 1537, &nsize)) != ERROR_SUCCESS){
        return ret;
    }

    char* s0s1 = buf;
    if((ret = socket->ReadFully(s0s1, 1537, &nsize)) != ERROR_SUCCESS){
        return ret;
    }
    char* s2 = buf;
    if((ret = socket->ReadFully(s2, 1536, &nsize)) != ERROR_SUCCESS){
        return ret;
    }
    
    char* c2 = buf;
    if((ret = socket->Write(c2, 1536, &nsize)) != ERROR_SUCCESS){
        return ret;
    }
    
    return ret;
}

int StRtmpClient::ConnectApp(RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    return ret;
}

int StRtmpClient::PlayStram(RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    return ret;
}

int StRtmpClient::DumpAV(){
    int ret = ERROR_SUCCESS;
    return ret;
}

