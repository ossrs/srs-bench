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

int StHttpTask::ProcessHttp(){
    int ret = ERROR_SUCCESS;
    
    Trace("start to process HTTP task #%d, schema=%s, host=%s, port=%d, path=%s, startup=%.2f, delay=%.2f, error=%.2f, count=%d", 
        GetId(), url.GetSchema(), url.GetHost(), url.GetPort(), url.GetPath(), startup_seconds, delay_seconds, error_seconds, count);
        
    StHttpClient client;
    
    // if count is zero, infinity loop.
    for(int i = 0; count == 0 || i < count; i++){
        if((ret = client.DownloadString(&url, NULL)) != ERROR_SUCCESS){
            Error("http client get url failed. ret=%d", ret);
            st_usleep(error_seconds * 1000 * 1000);
            continue;
        }
        
        int sleep_ms = StUtility::BuildRandomMTime(delay_seconds);
        Trace("[HTTP] %s download, size=%"PRId64", sleep %dms", url.GetUrl(), client.GetResponseHeader()->content_length, sleep_ms);
        st_usleep(sleep_ms * 1000);
    }
    
    return ret;
}
