#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <stdlib.h>

#include <string>
#include <sstream>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>
#include <htl_app_rtmp_client.hpp>

#include <htl_app_rtmp_load.hpp>

StRtmpTask::StRtmpTask(){
}

StRtmpTask::~StRtmpTask(){
}

int StRtmpTask::Initialize(std::string http_url, double startup, double delay, double error, int count){
    int ret = ERROR_SUCCESS;
    
    if((ret = InitializeBase(http_url, startup, delay, error, count)) != ERROR_SUCCESS){
        return ret;
    }
    
    return ret;
}

Uri* StRtmpTask::GetUri(){
    return &url;
}

int StRtmpTask::ProcessTask(){
    int ret = ERROR_SUCCESS;
    
    Trace("start to process RTMP task #%d, schema=%s, host=%s, port=%d, tcUrl=%s, stream=%s, startup=%.2f, delay=%.2f, error=%.2f, count=%d", 
        GetId(), url.GetSchema(), url.GetHost(), url.GetPort(), url.GetTcUrl(), url.GetStream(), startup_seconds, delay_seconds, error_seconds, count);
       
    StRtmpClient client;
    
    // if count is zero, infinity loop.
    for(int i = 0; count == 0 || i < count; i++){
        statistic->OnTaskStart(GetId(), url.GetUrl());
        
        if((ret = client.Dump(&url)) != ERROR_SUCCESS){
            statistic->OnTaskError(GetId(), 0);
            
            Error("rtmp client dump url failed. ret=%d", ret);
            st_usleep(error_seconds * 1000 * 1000);
            continue;
        }
        
        int sleep_ms = StUtility::BuildRandomMTime((delay_seconds >= 0)? delay_seconds:0);
        Trace("[RTMP] %s dump success, sleep %dms", url.GetUrl(), sleep_ms);
        st_usleep(sleep_ms * 1000);
        
        statistic->OnTaskEnd(GetId(), 0);
    }
    
    return ret;
}

