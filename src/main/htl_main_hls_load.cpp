#include <htl_stdinc.hpp>

#include <getopt.h>
#include <stdlib.h>

#include <string>
using namespace std;

// project lib
#include <htl_core_log.hpp>
#include <htl_core_error.hpp>
#include <htl_app_hls_load.hpp>

// global instance for graceful log.
LogContext* context = new StLogContext();

#define DefaultHttpUrl "http://192.168.2.111:3080/hls/hls.m3u8"
#define DefaultThread 1
#define DefaultDelaySeconds -1
#define DefaultErrorSeconds 10.0
#define DefaultCount 0

int discovery_options(int argc, char** argv, bool& show_help, bool& show_version, string& url, int& threads, double& delay, double& error, int& count){
    int ret = ERROR_SUCCESS;
    
    static option long_options[] = {
        {"threads", required_argument, 0, 't'},
        {"url", required_argument, 0, 'u'},
        {"delay", required_argument, 0, 'd'},
        {"count", required_argument, 0, 'c'},
        {"error", required_argument, 0, 'e'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    int opt = 0;
    int option_index = 0;
    while((opt = getopt_long(argc, argv, "hvu:t:d:c:e:", long_options, &option_index)) != -1){
        switch(opt){
            case 'h':
                show_help = true;
                break;
            case 'v':
                show_version = true;
                break;
            case 'u':
                url = optarg;
                break;
            case 't':
                threads = atoi(optarg);
                break;
            case 'd':
                delay = atof(optarg);
                break;
            case 'c':
                count = atoi(optarg);
                break;
            case 'e':
                error = atof(optarg);
                break;
            default:
                show_help = true;
                break;
        }
    }
    
    // check values
    if(url == ""){
        show_help = true;
        return ret;
    }

    return ret;
}

void help(char** argv){
    printf("%s, Copyright (c) 2013 winlin\n", ProductHTTPName);
    
    printf(""
        "Usage: %s <Options> <-u URL>\n"
        "%s base on st(state-threads), support huge concurrency.\n"
        "Options:\n"
        "  -t THREAD, --thread THREAD  The thread to start. defaut to %d\n"
        "  -u URL, --url URL           The load test http url. ie. %s\n"
        "  -d DELAY, --delay DELAY     The delay is the ramdom sleep when success in seconds. -1 to use ts duration. 0 means no delay. defaut to %.2f\n"
        "  -c COUNT, --count COUNT     The count is the number of downloads. 0 means infinity. defaut to %d\n"
        "  -e ERROR, --error ERROR     The error is the ramdom sleep when error in seconds. 0 means no delay. defaut to %.2f\n"
        "  -v, --version               Print the version and exit.\n"
        "  -h, --help                  Print this help message and exit.\n"
        "\n"
        "This program built for %s.\n"
        "Report bugs to <%s>\n",
        argv[0], argv[0], DefaultThread, DefaultHttpUrl, (double)DefaultDelaySeconds, DefaultCount, 
        DefaultErrorSeconds, BuildPlatform, BugReportEmail);
        
    exit(0);
}

void version(){
    printf(ProductVersion);
    exit(0);
}

int main(int argc, char** argv){
    int ret = ERROR_SUCCESS;
    
    StFarm farm;
    
    if((ret = farm.Initialize()) != ERROR_SUCCESS){
        Error("initialize the farm failed. ret=%d", ret);
        return ret;
    }
    
    bool show_help = false, show_version = false; 
    string url; int threads = DefaultThread; double delay = DefaultDelaySeconds, error = DefaultErrorSeconds; int count = DefaultCount;
    if((ret = discovery_options(argc, argv, show_help, show_version, url, threads, delay, error, count)) != ERROR_SUCCESS){
        Error("discovery options failed. ret=%d", ret);
        return ret;
    }
    
    if(show_help){
        help(argv);
    }
    if(show_version){
        version();
    }

    for(int i = 0; i < threads; i++){
        StHlsTask* task = new StHlsTask();

        if((ret = task->Initialize(url, delay, error, count)) != ERROR_SUCCESS){
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
