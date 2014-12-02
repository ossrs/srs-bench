#ifndef _htl_app_rtmp_publish_hpp
#define _htl_app_rtmp_publish_hpp

/*
#include <htl_app_rtmp_publish.hpp>
*/
#include <string>

#include <htl_core_uri.hpp>
#include <htl_os_st.hpp>

#include <htl_app_rtmp_protocol.hpp>

class StRtmpPublishClient
{
private:
    srs_rtmp_t srs;
    int stream_id;
public:
    StRtmpPublishClient();
    virtual ~StRtmpPublishClient();
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
