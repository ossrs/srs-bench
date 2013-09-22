#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <stdlib.h>

#include <string>
#include <sstream>
#include <vector>
using namespace std;

#include <htl_core_log.hpp>
#include <htl_core_error.hpp>
#include <htl_core_aggregate_ret.hpp>
#include <htl_app_http_client.hpp>

#include <htl_app_hls_load.hpp>

#include <algorithm>

#define DEFAULT_TS_DURATION 10

StHlsTask::StHlsTask(){
    target_duration = DEFAULT_TS_DURATION;
}

StHlsTask::~StHlsTask(){
}

int StHlsTask::Initialize(std::string http_url, bool vod, double startup, double delay, double error, int count){
    int ret = ERROR_SUCCESS;
    
    is_vod = vod;
    
    if((ret = InitializeBase(http_url, startup, delay, error, count)) != ERROR_SUCCESS){
        return ret;
    }
    
    return ret;
}

int StHlsTask::ProcessHttp(){
    int ret = ERROR_SUCCESS;
    
    Trace("start to process HLS task #%d, schema=%s, host=%s, port=%d, path=%s, startup=%.2f, delay=%.2f, error=%.2f, count=%d", 
        GetId(), url.GetSchema(), url.GetHost(), url.GetPort(), url.GetPath(), startup_seconds, delay_seconds, error_seconds, count);
        
    StHttpClient client;
    
    // if count is zero, infinity loop.
    for(int i = 0; count == 0 || i < count; i++){
        statistic->OnTaskStart(GetId(), url.GetUrl());
        
        if((ret = ProcessM3u8(client)) != ERROR_SUCCESS){
            statistic->OnTaskError(GetId());
            
            Error("http client process m3u8 failed. ret=%d", ret);
            st_usleep(error_seconds * 1000 * 1000);
            continue;
        }
        
        int sleep_ms = StUtility::BuildRandomMTime(delay_seconds);
        Info("[HLS] %s download, sleep %dms", url.GetUrl(), sleep_ms);
        st_usleep(sleep_ms * 1000);
        
        statistic->OnTaskEnd(GetId(), 0);
    }
    
    return ret;
}

int StHlsTask::ProcessM3u8(StHttpClient& client){
    int ret = ERROR_SUCCESS;
    
    string m3u8;
    if((ret = client.DownloadString(&url, &m3u8)) != ERROR_SUCCESS){
        Error("http client get m3u8 failed. ret=%d", ret);
        return ret;
    }
    Trace("[HLS] get m3u8 %s get success, length=%"PRId64, url.GetUrl(), m3u8.length());
    
    vector<M3u8TS> ts_objects;
    if((ret = HlsM3u8Parser::ParseM3u8Data(&url, m3u8, ts_objects, target_duration)) != ERROR_SUCCESS){
        Error("http client parse m3u8 content failed. ret=%d", ret);
        return ret;
    }
    
    if((ret = ProcessTS(client, ts_objects)) != ERROR_SUCCESS){
        Error("http client download m3u8 ts file failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int StHlsTask::ProcessTS(StHttpClient& client, vector<M3u8TS>& ts_objects){
    int ret = ERROR_SUCCESS;
    
    vector<M3u8TS>::iterator ite = ts_objects.begin();
    
    // if live(not vod), remember the last download ts object.
    // if vod(not live), always access from the frist ts.
    if(!is_vod){
        ite = find(ts_objects.begin(), ts_objects.end(), last_downloaded_ts);
        
        // not found, reset to begin to process all.
        if(ite == ts_objects.end()){
            ite = ts_objects.begin();
        }
        // fount, skip it.
        else{
            ite++;
        }
        
        // no ts now, wait for a segment
        if(ite == ts_objects.end()){
            int sleep_ms = StUtility::BuildRandomMTime((target_duration > 0)? target_duration:DEFAULT_TS_DURATION);
            Trace("[TS] no fresh ts, wait for a while. sleep %dms", sleep_ms);
            st_usleep(sleep_ms * 1000);
            
            return ret;
        }
    }
    
    AggregateRet aggregate_ret;
    
    // to process from the specified ite
    for(; ite != ts_objects.end(); ++ite){
        M3u8TS ts_object = *ite;

        if(!is_vod){
            last_downloaded_ts = ts_object;
        }
        
        Info("start to process ts %s", ts_object.ts_url.c_str());
        
        aggregate_ret.Add(DownloadTS(client, ts_object));
    }
    
    return aggregate_ret.GetReturnValue();
}

int StHlsTask::DownloadTS(StHttpClient& client, M3u8TS& ts){
    int ret = ERROR_SUCCESS;
    
    HttpUrl url;
    
    if((ret = url.Initialize(ts.ts_url)) != ERROR_SUCCESS){
        Error("initialize ts url failed. ret=%d", ret);
        return ret;
    }
    
    Info("[TS] url=%s, duration=%.2f, delay=%.2f", url.GetUrl(), ts.duration, delay_seconds);
    statistic->OnSubTaskStart(GetId(), ts.ts_url);
    
    if((ret = client.DownloadString(&url, NULL)) != ERROR_SUCCESS){
        statistic->OnSubTaskError(GetId());
            
        Error("http client download ts file %s failed. ret=%d", url.GetUrl(), ret);
        return ret;
    }
    
    int sleep_ms = StUtility::BuildRandomMTime((delay_seconds >= 0)? delay_seconds:ts.duration);
    Trace("[TS] url=%s download, duration=%.2f, delay=%.2f, size=%"PRId64", sleep %dms", 
        url.GetUrl(), ts.duration, delay_seconds, client.GetResponseHeader()->content_length, sleep_ms);
    st_usleep(sleep_ms * 1000);
    
    statistic->OnSubTaskEnd(GetId(), ts.duration);
    
    return ret;
}
