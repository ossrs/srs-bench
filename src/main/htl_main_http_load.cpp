#include <htl_stdinc.hpp>

// system
#include <unistd.h>
#include <errno.h>

// socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// c lib
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// c++ lib
#include <string>
#include <sstream>
using namespace std;

// 3rdparty lib
#include <http_parser.h>

// project lib
#include <htl_core_log.hpp>
#include <htl_core_error.hpp>
#include <htl_os_st.hpp>
#include <htl_app_http_load.hpp>

// global instance for graceful log.
LogContext* context = new StLogContext();

int main(int /*argc*/, char** /*argv*/){
    int ret = ERROR_SUCCESS;
    
    StFarm farm;
    
    if((ret = farm.Initialize()) != ERROR_SUCCESS){
        Error("initialize the farm failed. ret=%d", ret);
        return ret;
    }

    for(int i = 0; i < 1; i++){
        string url;
        url = "http://192.168.2.111:3080/hls/hls.m3u8";
        //url = "http://192.168.2.111:3080/hls/segm130813144315787-522881.ts";
    
        StHttpTask* task = new StHttpTask();

        if((ret = task->Initialize(url)) != ERROR_SUCCESS){
            Error("initialize task failed, url=%s, ret=%d", url.c_str(), ret);
            return ret;
        }
        
        if((ret = farm.Spawn(task)) != ERROR_SUCCESS){
            Error("st farm spwan task failed, ret=%d", ret);
            return ret;
        }
    }
    
    farm.WaitAll();
    
    return 0;
}
