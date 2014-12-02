#ifndef _htl_app_rtmp_client_hpp
#define _htl_app_rtmp_client_hpp

/*
#include <htl_app_rtmp_client.hpp>
*/
#include <string>

#include <htl_core_uri.hpp>
#include <htl_os_st.hpp>

#include <htl_app_rtmp_protocol.hpp>

class StRtmpClient
{
private:
    srs_rtmp_t srs;
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
    virtual int ConnectApp();
    virtual int PlayStram();
    virtual int DumpAV();
};

#endif
