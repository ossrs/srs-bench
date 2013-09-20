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
        int sleep_ms = (int)(startup_seconds * 1000 * 0.8) + rand() % (int)(startup_seconds * 1000 * 0.4);
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
        
        int sleep_ms = 0;
        if(delay_seconds > 0){
            sleep_ms = (int)(delay_seconds * 1000 * 0.8) + rand() % (int)(delay_seconds * 1000 * 0.4);
        }
        
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
    if((ret = ParseM3u8Data(m3u8, ts_objects)) != ERROR_SUCCESS){
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
        int sleep_ms = DEFAULT_TS_DURATION;

        if(target_duration > 0){
            sleep_ms = (int)(target_duration * 1000 * 0.8) + rand() % (int)(target_duration * 1000 * 0.4);
        }
        
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
    
    int sleep_ms = 0;
    if(delay_seconds > 0){
        sleep_ms = (int)(delay_seconds * 1000 * 0.8) + rand() % (int)(delay_seconds * 1000 * 0.4);
    }
    if(delay_seconds == -1){
        sleep_ms = (int)(ts.duration * 1000 * 0.8) + rand() % (int)(ts.duration * 1000 * 0.4);
    }
    
    
    Trace("[TS] url=%s download, duration=%.2f, delay=%.2f, size=%"PRId64", sleep %dms", 
        url.GetUrl(), ts.duration, delay_seconds, client.GetResponseHeader()->content_length, sleep_ms);
    
    st_usleep(sleep_ms * 1000);
    
    return ret;
}

class String
{
private:
    string value;
public:
    String(string str = ""){
        value = str;
    }
public:
    String& set_str(string str){
        value = str;
        return *this;
    }
    int length(){
        return (int)value.length();
    }
    bool startswith(string key, String* left = NULL){
        size_t pos = value.find(key);
        
        if(pos == 0 && left != NULL){
            left->set_str(value.substr(pos + key.length()));
            left->strip();
        }
        
        return pos == 0;
    }
    bool endswith(string key, String* left = NULL){
        size_t pos = value.rfind(key);
        
        if(pos == value.length() - key.length() && left != NULL){
            left->set_str(value.substr(pos));
            left->strip();
        }
        
        return pos == value.length() - key.length();
    }
    String& strip(){
        while(value.length() > 0){
            if(startswith("\n")){
                value = value.substr(1);
                continue;
            }
            
            if(startswith("\r")){
                value = value.substr(1);
                continue;
            }
            
            if(startswith(" ")){
                value = value.substr(1);
                continue;
            }
            
            if(endswith("\n")){
                value = value.substr(0, value.length() - 1);
                continue;
            }
            
            if(endswith("\r")){
                value = value.substr(0, value.length() - 1);
                continue;
            }
            
            if(endswith(" ")){
                value = value.substr(0, value.length() - 1);
                continue;
            }

            break;
        }
        
        return *this;
    }
    string getline(){
        size_t pos = string::npos;
        
        if((pos = value.find("\n")) != string::npos){
            return value.substr(0, pos);
        }
        
        return value;
    }
    String& remove(int size){
        if(size >= (int)value.length()){
            value = "";
            return *this;
        }
        
        value = value.substr(size);
        return *this;
    }
    const char* c_str(){
        return value.c_str();
    }
};

int StHlsTask::ParseM3u8Data(string m3u8, vector<M3u8TS>& ts_objects){
    int ret = ERROR_SUCCESS;
    
    String data(m3u8);
    
    // http://tools.ietf.org/html/draft-pantos-http-live-streaming-08#section-3.3.1
    // An Extended M3U file is distinguished from a basic M3U file by its
    // first line, which MUST be the tag #EXTM3U.
    if(!data.startswith("#EXTM3U")){
        ret = ERROR_HLS_INVALID;
        Error("invalid hls, #EXTM3U not found. ret=%d", ret);
        return ret;
    }
    
    String value;
    
    M3u8TS ts_object;
    while(data.length() > 0){
        String line;
        data.remove(line.set_str(data.strip().getline()).strip().length()).strip();

        // http://tools.ietf.org/html/draft-pantos-http-live-streaming-08#section-3.4.2
        // #EXT-X-TARGETDURATION:<s>
        // where s is an integer indicating the target duration in seconds.
        if(line.startswith("#EXT-X-TARGETDURATION:", &value)){
            target_duration = atoi(value.c_str());
            ts_object.duration = target_duration;
            continue;
        }
        
        // http://tools.ietf.org/html/draft-pantos-http-live-streaming-08#section-3.3.2
        // #EXTINF:<duration>,<title>
        // "duration" is an integer or floating-point number in decimal
        // positional notation that specifies the duration of the media segment
        // in seconds.  Durations that are reported as integers SHOULD be
        // rounded to the nearest integer.  Durations MUST be integers if the
        // protocol version of the Playlist file is less than 3.  The remainder
        // of the line following the comma is an optional human-readable
        // informative title of the media segment.
        // ignore others util EXTINF
        if(line.startswith("#EXTINF:", &value)){
            ts_object.duration = atof(value.c_str());
            continue;
        }
        
        if(!line.startswith("#")){
            ts_object.ts_url = url.Resolve(line.c_str());
            ts_objects.push_back(ts_object);
            continue;
        }
    }
    
    return ret;
}
