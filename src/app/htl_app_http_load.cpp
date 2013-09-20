#include <string>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>

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
    
    Trace("start to process task #%d, host=%s, port=%d, path=%s", GetId(), url.GetHost(), url.GetPort(), url.GetPath());
    st_usleep(-1);
    
    return ret;
}
