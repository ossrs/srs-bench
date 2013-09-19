#include <string>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>

#include <htl_app_http_load.hpp>

StHttpTask::StHttpTask(string http_url){
    url = http_url;
}

StHttpTask::~StHttpTask(){
}

int StHttpTask::Process(){
    Trace("start to process task #%d", GetId());
    st_usleep(-1);
    
    return ERROR_SUCCESS;
}
