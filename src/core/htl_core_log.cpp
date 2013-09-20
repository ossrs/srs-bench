#include <htl_stdinc.hpp>

#include <string.h>
#include <sys/time.h>

#include <htl_core_log.hpp>

DateTime::DateTime(){
    memset(time_data, 0, DATE_LEN);
}

DateTime::~DateTime(){
}

const char* DateTime::FormatTime(){
    // clock time
    timeval tv;
    if(gettimeofday(&tv, NULL) == -1){
        return "";
    }
    // to calendar time
    struct tm* tm;
    if((tm = localtime(&tv.tv_sec)) == NULL){
        return "";
    }
    
    // log header, the time/pid/level of log
    // reserved 1bytes for the new line.
    snprintf(time_data, DATE_LEN, "%d-%02d-%02d %02d:%02d:%02d.%03d", 
        1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, 
        (int)(tv.tv_usec / 1000));
        
    return time_data;
}

LogContext::LogContext(){
}

LogContext::~LogContext(){
}

int LogContext::GetId(){
    return 0;
}

const char* LogContext::FormatTime(){
    return time.FormatTime();
}
