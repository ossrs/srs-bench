#ifndef _htl_app_rtmp_client_hpp
#define _htl_app_rtmp_client_hpp

/*
#include <htl_app_rtmp_client.hpp>
*/
#include <string>

#include <htl_core_uri.hpp>
#include <htl_os_st.hpp>

class StRtmpClient
{
private:
    StSocket* socket;
public:
    StRtmpClient();
    virtual ~StRtmpClient();
public:
    virtual int Dump(RtmpUrl* url);
};

#endif