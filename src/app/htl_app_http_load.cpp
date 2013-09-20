#include <htl_stdinc.hpp>

#include <inttypes.h>

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

int StHttpTask::Initialize(std::string http_url){
    int ret = ERROR_SUCCESS;
    
    if((ret = url.Initialize(http_url)) != ERROR_SUCCESS){
        return ret;
    }
    
    Info("url(%s) parsed", http_url.c_str());
    
    return ret;
}

int StHttpTask::Process(){
    int ret = ERROR_SUCCESS;
    
    Trace("start to process task #%d, schema=%s, host=%s, port=%d, path=%s", 
        GetId(), url.GetSchema(), url.GetHost(), url.GetPort(), url.GetPath());
        
    StHttpClient client;
    
    while(true){
        if((ret = client.DownloadString(&url, NULL)) != ERROR_SUCCESS){
            Error("http client get url failed. ret=%d", ret);
            return ret;
        }
        
        Trace("[HTTP] %s download, size=%"PRId64, url.GetUrl(), client.GetResponseHeader()->content_length);
        
        st_usleep(3 * 1000 * 1000);
    }
    
    return ret;
}
