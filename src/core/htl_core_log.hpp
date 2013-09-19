#ifndef _htl_core_log_hpp
#define _htl_core_log_hpp

/*
#include <htl_core_log.hpp>
*/

#include <stdio.h>

#include <errno.h>
#include <string.h>

#if 0
    #define Verbose(msg, ...) (void)0
    #define Info(msg, ...) (void)0
    #define Trace(msg, ...) (void)0
    #define Warn(msg, ...) (void)0
    #define Error(msg, ...) (void)0
#else
    #define Verbose(msg, ...) printf("[verbs] [%s] ", __FUNCTION__);printf(msg, ##__VA_ARGS__);printf("\n")
    #define Info(msg, ...)    printf("[infos] ");printf(msg, ##__VA_ARGS__);printf("\n")
    #define Trace(msg, ...)   printf("[trace] ");printf(msg, ##__VA_ARGS__);printf("\n")
    #define Warn(msg, ...)    printf("[warns] ");printf(msg, ##__VA_ARGS__);printf(" errno=%d(%s)", errno, strerror(errno));printf("\n")
    #define Error(msg, ...)   printf("[error] ");printf(msg, ##__VA_ARGS__);printf(" errno=%d(%s)", errno, strerror(errno));printf("\n")
#endif

#if 0
    #undef Info
    #define Info(msg, ...) (void)0
#endif

#endif