#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <stdlib.h>

#include <string>
#include <sstream>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>
#include <htl_app_http_client.hpp>

#include <htl_app_http_load.hpp>

StHttpTask::StHttpTask(){
}

StHttpTask::~StHttpTask(){
}

int StHttpTask::Initialize(std::string http_url, double delay, double error, int count){
    int ret = ERROR_SUCCESS;
    
    if((ret = url.Initialize(http_url)) != ERROR_SUCCESS){
        return ret;
    }
    
    Info("task url(%s) parsed, delay=%.2f, error=%.2f, count=%d", http_url.c_str(), delay, error, count);
    
    this->delay_seconds = delay;
    this->error_seconds = error;
    this->count = count;
    
    return ret;
}

int StHttpTask::Process(){
    int ret = ERROR_SUCCESS;
    
    Trace("start to process task #%d, schema=%s, host=%s, port=%d, path=%s, delay=%.2f, error=%.2f, count=%d", 
        GetId(), url.GetSchema(), url.GetHost(), url.GetPort(), url.GetPath(), delay_seconds, error_seconds, count);
        
    StHttpClient client;
    
    // if count is zero, infinity loop.
    for(int i = 0; count == 0 || i < count; i++){
        if((ret = client.DownloadString(&url, NULL)) != ERROR_SUCCESS){
            Error("http client get url failed. ret=%d", ret);
            st_usleep(error_seconds * 1000 * 1000);
            continue;
        }
        
        int sleep_ms = 0;
        if(delay_seconds > 0){
            sleep_ms = (int)(delay_seconds * 1000 * 0.8) + rand() % (int)(delay_seconds * 1000 * 0.4);
        }
        
        Trace("[HTTP] %s download, size=%"PRId64", sleep %dms", url.GetUrl(), client.GetResponseHeader()->content_length, sleep_ms);
        
        st_usleep(sleep_ms * 1000);
    }
    
    return ret;
}
