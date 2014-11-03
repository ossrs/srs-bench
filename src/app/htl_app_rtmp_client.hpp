#ifndef _htl_app_rtmp_client_hpp
#define _htl_app_rtmp_client_hpp

/*
#include <htl_app_rtmp_client.hpp>
*/
#include <string>

#include <htl_core_uri.hpp>
#include <htl_os_st.hpp>

class SrsRtmpClient;

class StRtmpClient
{
private:
	SrsRtmpClient* srs;
	StSocket* socket;
    int stream_id;
public:
    StRtmpClient();
    virtual ~StRtmpClient();
public:
    virtual int Dump(RtmpUrl* url);
private:
    virtual int Connect(RtmpUrl* url);
    virtual int Handshake();
    virtual int ConnectApp(RtmpUrl* url);
    virtual int CreateStream();
    virtual int PlayStram(RtmpUrl* url);
    virtual int DumpAV();
};

#endif
