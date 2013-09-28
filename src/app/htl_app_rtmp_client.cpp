#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <assert.h>

#include <string>
#include <sstream>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>

#include <htl_app_rtmp_client.hpp>

StRtmpClient::StRtmpClient(){
    socket = new StSocket();
}

StRtmpClient::~StRtmpClient(){
    delete socket;
    socket = NULL;
}

int StRtmpClient::Dump(RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    return ret;
}

