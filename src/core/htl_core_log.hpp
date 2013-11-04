#ifndef _htl_core_log_hpp
#define _htl_core_log_hpp

/*
#include <htl_core_log.hpp>
*/

#include <stdio.h>

#include <errno.h>
#include <string.h>

#include <string>

class DateTime
{
private:
    // %d-%02d-%02d %02d:%02d:%02d.%03d
    #define DATE_LEN 24
    char time_data[DATE_LEN];
public:
    DateTime();
    virtual ~DateTime();
public:
    virtual const char* FormatTime();
};

class LogContext
{
private:
    DateTime time;
public:
    LogContext();
    virtual ~LogContext();
public:
    virtual void SetId(int id) = 0;
    virtual int GetId();
public:
    virtual const char* FormatTime();
};

// user must implements the LogContext and define a global instance.
extern LogContext* context;

#if 1
    #define Verbose(msg, ...) printf("[%s][%d][verbs] ", context->FormatTime(), context->GetId());printf(msg, ##__VA_ARGS__);printf("\n")
    #define Info(msg, ...)    printf("[%s][%d][infos] ", context->FormatTime(), context->GetId());printf(msg, ##__VA_ARGS__);printf("\n")
    #define Trace(msg, ...)   printf("[%s][%d][trace] ", context->FormatTime(), context->GetId());printf(msg, ##__VA_ARGS__);printf("\n")
    #define Warn(msg, ...)    printf("[%s][%d][warns] ", context->FormatTime(), context->GetId());printf(msg, ##__VA_ARGS__);printf(" errno=%d(%s)", errno, strerror(errno));printf("\n")
    #define Error(msg, ...)   printf("[%s][%d][error] ", context->FormatTime(), context->GetId());printf(msg, ##__VA_ARGS__);printf(" errno=%d(%s)", errno, strerror(errno));printf("\n")
#else
    #define Verbose(msg, ...) printf("[%s][%d][verbs][%s] ", context->FormatTime(), context->GetId(), __FUNCTION__);printf(msg, ##__VA_ARGS__);printf("\n")
    #define Info(msg, ...)    printf("[%s][%d][infos][%s] ", context->FormatTime(), context->GetId(), __FUNCTION__);printf(msg, ##__VA_ARGS__);printf("\n")
    #define Trace(msg, ...)   printf("[%s][%d][trace][%s] ", context->FormatTime(), context->GetId(), __FUNCTION__);printf(msg, ##__VA_ARGS__);printf("\n")
    #define Warn(msg, ...)    printf("[%s][%d][warns][%s] ", context->FormatTime(), context->GetId(), __FUNCTION__);printf(msg, ##__VA_ARGS__);printf(" errno=%d(%s)", errno, strerror(errno));printf("\n")
    #define Error(msg, ...)   printf("[%s][%d][error][%s] ", context->FormatTime(), context->GetId(), __FUNCTION__);printf(msg, ##__VA_ARGS__);printf(" errno=%d(%s)", errno, strerror(errno));printf("\n")
#endif

#if 1
    #undef Verbose
    #define Verbose(msg, ...) (void)0
#endif
#if 1
    #undef Info
    #define Info(msg, ...) (void)0
#endif

// for summary/report thread, print to stderr.
#define LReport(msg, ...)   fprintf(stderr, "[%s] ", context->FormatTime());fprintf(stderr, msg, ##__VA_ARGS__);fprintf(stderr, "\n")
    
#endif
