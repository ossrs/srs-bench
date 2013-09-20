#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <stdlib.h>

#include <string>
#include <sstream>
#include <vector>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>
#include <htl_app_http_client.hpp>

#include <htl_app_hls_load.hpp>

#include <algorithm>

#define DEFAULT_TS_DURATION 10

StHlsTask::StHlsTask(){
    target_duration = DEFAULT_TS_DURATION;
}

StHlsTask::~StHlsTask(){
}

int StHlsTask::Initialize(std::string http_url, double startup, double delay, double error, int count){
    int ret = ERROR_SUCCESS;
    
    if((ret = url.Initialize(http_url)) != ERROR_SUCCESS){
        return ret;
    }
    
    Info("task url(%s) parsed, startup=%.2f, delay=%.2f, error=%.2f, count=%d", http_url.c_str(), startup, delay, error, count);
    
    this->delay_seconds = delay;
    this->startup_seconds = startup;
    this->error_seconds = error;
    this->count = count;
    
    return ret;
}

int StHlsTask::Process(){
    int ret = ERROR_SUCCESS;
    
    Trace("start to process HLS task #%d, schema=%s, host=%s, port=%d, path=%s, startup=%.2f, delay=%.2f, error=%.2f, count=%d", 
        GetId(), url.GetSchema(), url.GetHost(), url.GetPort(), url.GetPath(), startup_seconds, delay_seconds, error_seconds, count);
    
    if(startup_seconds > 0){
        int sleep_ms = StUtility::BuildRandomMTime(startup_seconds);
        Trace("start random sleep %dms", sleep_ms);
        st_usleep(sleep_ms * 1000);
    }
        
    StHttpClient client;
    
    // if count is zero, infinity loop.
    for(int i = 0; count == 0 || i < count; i++){
        if((ret = ProcessM3u8(client)) != ERROR_SUCCESS){
            Error("http client process m3u8 failed. ret=%d", ret);
            st_usleep(error_seconds * 1000 * 1000);
            continue;
        }
        
        int sleep_ms = StUtility::BuildRandomMTime(delay_seconds);
        Info("[HLS] %s download, sleep %dms", url.GetUrl(), sleep_ms);
        st_usleep(sleep_ms * 1000);
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
    
    vector<M3u8TS>::iterator ite = find(ts_objects.begin(), ts_objects.end(), last_downloaded_ts);
    
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
        int sleep_ms = StUtility::BuildRandomMTime(target_duration, DEFAULT_TS_DURATION);
        Trace("[TS] no fresh ts, wait for a while. sleep %dms", sleep_ms);
        st_usleep(sleep_ms * 1000);
        
        return ret;
    }
    
    // to process from the specified ite
    for(; ite != ts_objects.end(); ++ite){
        M3u8TS ts_object = *ite;
        last_downloaded_ts = ts_object;
        
        Info("start to process ts %s", ts_object.ts_url.c_str());
        
        if((ret = DownloadTS(client, ts_object)) != ERROR_SUCCESS){
            Error("process ts file failed. ret=%d", ret);
            return ret;
        }
    }
    
    return ret;
}

int StHlsTask::DownloadTS(StHttpClient& client, M3u8TS& ts){
    int ret = ERROR_SUCCESS;
    
    HttpUrl url;
    
    if((ret = url.Initialize(ts.ts_url)) != ERROR_SUCCESS){
        Error("initialize ts url failed. ret=%d", ret);
        return ret;
    }
    
    Info("[TS] url=%s, duration=%.2f, delay=%.2f", url.GetUrl(), ts.duration, delay_seconds);
    
    if((ret = client.DownloadString(&url, NULL)) != ERROR_SUCCESS){
        Error("http client download ts file %s failed. ret=%d", url.GetUrl(), ret);
        return ret;
    }
    
    int sleep_ms = StUtility::BuildRandomMTime(delay_seconds, (delay_seconds == -1)? ts.duration : 0);
    Trace("[TS] url=%s download, duration=%.2f, delay=%.2f, size=%"PRId64", sleep %dms", 
        url.GetUrl(), ts.duration, delay_seconds, client.GetResponseHeader()->content_length, sleep_ms);
    st_usleep(sleep_ms * 1000);
    
    return ret;
}
