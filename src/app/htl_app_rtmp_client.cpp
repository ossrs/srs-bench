#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <assert.h>

#include <string>
#include <sstream>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>

#include <htl_app_rtmp_client.hpp>
#include <htl_app_rtmp_protocol.hpp>

StRtmpClient::StRtmpClient(){
    socket = new StSocket();
    stream_id = 0;
    srs = NULL;
}

StRtmpClient::~StRtmpClient(){
    delete socket;
    delete srs;
    socket = NULL;
}

int StRtmpClient::Dump(RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    
    delete socket;
    socket = new StSocket();
    
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
    
    if((ret = CreateStream()) != ERROR_SUCCESS){
        Error("rtmp client create stream failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client create stream(%d) success", stream_id);
    
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
    
	delete srs;
    srs = new SrsRtmpClient(socket->GetStfd());
    
    return ret;
}

int StRtmpClient::Handshake(){
    return srs->handshake();
}

int StRtmpClient::ConnectApp(RtmpUrl* url){
    return srs->connect_app(url->GetApp(), url->GetTcUrl());
}

int StRtmpClient::CreateStream(){
	return srs->create_stream(stream_id);
}

int StRtmpClient::PlayStram(RtmpUrl* url){
	return srs->play(url->GetStream(), stream_id);
}

int StRtmpClient::DumpAV(){
    int ret = ERROR_SUCCESS;

    // recv response
    while(true){
        SrsCommonMessage* msg = NULL;
        if ((ret = srs->recv_message(&msg)) != ERROR_SUCCESS){
            return ret;
        }
		SrsAutoFree(SrsCommonMessage, msg, false);
        
        Info("get message type=%d, size=%d", 
        	msg->header.message_type, msg->header.payload_length);
    }
    
    return ret;
}
