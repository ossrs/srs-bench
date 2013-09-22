#ifndef _htl_main_utility_hpp
#define _htl_main_utility_hpp

/*
#include <htl_main_utility.hpp>
*/

// global instance for graceful log.
LogContext* context = new StLogContext();

#define DefaultThread 1
#define DefaultStartupSeconds 5.0
#define DefaultErrorSeconds 10.0
#define DefaultReportSeconds 30.0
#define DefaultCount 0

#define SharedOptions()\
        {"threads", required_argument, 0, 't'}, \
        {"url", required_argument, 0, 'u'}, \
        {"startup", required_argument, 0, 's'}, \
        {"delay", required_argument, 0, 'd'}, \
        {"count", required_argument, 0, 'c'}, \
        {"error", required_argument, 0, 'e'}, \
        {"report", required_argument, 0, 'r'}, \
        {"help", no_argument, 0, 'h'}, \
        {"version", no_argument, 0, 'v'},

#define ProcessSharedOptions()\
            case 'h': \
                show_help = true; \
                break; \
            case 'v': \
                show_version = true; \
                break; \
            case 'u': \
                url = optarg; \
                break; \
            case 't': \
                threads = atoi(optarg); \
                break; \
            case 's': \
                startup = atof(optarg); \
                break; \
            case 'd': \
                delay = atof(optarg); \
                break; \
            case 'c': \
                count = atoi(optarg); \
                break; \
            case 'e': \
                error = atof(optarg); \
                break; \
            case 'r': \
                report = atof(optarg); \
                break;

#define ShowHelpPart1()\
        "  -t THREAD, --thread THREAD  The thread to start. defaut: %d\n" \
        "  -u URL, --url URL           The load test http url. \n" \
        "                              ie. %s\n"
#define ShowHelpPart2()\
        "  -s STARTUP, --start STARTUP The start is the ramdom sleep when  thread startup in seconds. \n" \
        "                              defaut: %.2f. 0 means no delay.\n" \
        "  -d DELAY, --delay DELAY     The delay is the ramdom sleep when success in seconds. \n" \
        "                              default: %.2f. 0 means no delay. -1 means to use HLS EXTINF duration(HLS only).\n" \
        "  -c COUNT, --count COUNT     The count is the number of downloads. \n" \
        "                              default: %d. 0 means infinity.\n" \
        "  -e ERROR, --error ERROR     The error is the sleep when error in seconds. \n" \
        "                              defaut: %.2f. 0 means no delay. \n" \
        "  -r REPORT, --report REPORT  The report is the sleep when report in seconds. \n" \
        "                              etasks is error_tasks, statks is sub_tasks, estatks is error_sub_tasks.\n" \
        "                              nread/nwrite in Mbps, duration in seconds.\n" \
        "                              defaut: %.2f. 0 means no delay. \n" \
        "  -v, --version               Print the version and exit.\n" \
        "  -h, --help                  Print this help message and exit.\n"

void version(){
    printf(ProductVersion);
    exit(0);
}

#endif
