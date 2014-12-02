#include <htl_stdinc.hpp>

#include <getopt.h>
#include <stdlib.h>

#include <string>
using namespace std;

// project lib
#include <htl_core_log.hpp>
#include <htl_core_error.hpp>
#include <htl_app_rtmp_load.hpp>

#include <htl_main_utility.hpp>

#define DefaultDelaySeconds 1.0
#define DefaultRtmpUrl "rtmp://127.0.0.1:1935/live/livestream"
#define DefaultInputFlv "~/srs/doc/source.200kbps.768x320.flv"

int discovery_options(int argc, char** argv, 
    bool& show_help, bool& show_version, string& url, int& threads, 
    double& startup, double& delay, double& error, double& report, int& count,
    string& input
){
    int ret = ERROR_SUCCESS;
    
    static option long_options[] = {
        SharedOptions()
        {"input", no_argument, 0, 'i'},
        {0, 0, 0, 0}
    };
    
    int opt = 0;
    int option_index = 0;
    while((opt = getopt_long(argc, argv, "hvc:r:t:s:d:e:m:i:", long_options, &option_index)) != -1){
        switch(opt){
            ProcessSharedOptions()
            case 'i':
                input = optarg;
                break;
            default:
                show_help = true;
                break;
        }
    }
    
    // check values
    if(url == "" || input == ""){
        show_help = true;
        return ret;
    }

    return ret;
}

void help(char** argv){
    printf("%s, Copyright (c) 2013 winlin\n", ProductRtmpPublishName);
    
    printf(""
        "Usage: %s <Options> <-u URL>\n"
        "%s base on st(state-threads), support huge concurrency.\n"
        "Options:\n"
        ShowHelpPart1()
        ShowHelpPart2()
        "\n"
        "Examples:\n"
        "1. start a client\n"
        "   %s -i %s -c 1 -r %s\n"
        "2. start 1000 clients\n"
        "   %s -i %s -c 1000 -r %s\n"
        "3. start 10000 clients\n"
        "   %s -i %s -c 10000 -r %s\n"
        "4. start 100000 clients\n"
        "   %s -i %s -c 100000 -r %s\n"
        "\n"
        "This program built for %s.\n"
        "Report bugs to <%s>\n",
        argv[0], argv[0], 
        DefaultThread, DefaultRtmpUrl, DefaultCount, // part1
        (double)DefaultStartupSeconds, DefaultDelaySeconds, // part2
        DefaultErrorSeconds, DefaultReportSeconds, // part2
        argv[0], DefaultInputFlv, DefaultRtmpUrl, 
        argv[0], DefaultInputFlv, DefaultRtmpUrl, 
        argv[0], DefaultInputFlv, DefaultRtmpUrl, 
        argv[0], DefaultInputFlv, DefaultRtmpUrl,
        BuildPlatform, BugReportEmail);
        
    exit(0);
}

int main(int argc, char** argv){
    int ret = ERROR_SUCCESS;
    
    bool show_help = false, show_version = false; 
    string url; int threads = DefaultThread; 
    double start = DefaultStartupSeconds, delay = DefaultDelaySeconds, error = DefaultErrorSeconds;
    double report = DefaultReportSeconds; int count = DefaultCount;
    string input;
    
    if((ret = discovery_options(argc, argv, show_help, show_version, url, threads, start, delay, error, report, count, input)) != ERROR_SUCCESS){
        Error("discovery options failed. ret=%d", ret);
        return ret;
    }
    
    if(show_help){
        help(argv);
    }
    if(show_version){
        version();
    }
    
    Trace("params url=%s, threads=%d, start=%.2f, delay=%.2f, error=%.2f, report=%.2f, count=%d", 
        url.c_str(), threads, start, delay, error, report, count);
    
    StFarm farm;
    
    if((ret = farm.Initialize(report)) != ERROR_SUCCESS){
        Error("initialize the farm failed. ret=%d", ret);
        return ret;
    }

    for(int i = 0; i < threads; i++){
        StRtmpTask* task = new StRtmpTask();

        if((ret = task->Initialize(url, start, delay, error, count)) != ERROR_SUCCESS){
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

