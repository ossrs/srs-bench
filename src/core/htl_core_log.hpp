#ifndef _hls_core_log_hpp
#define _hls_core_log_hpp

/*
#include <hls_core_log.hpp>
*/

#include <stdio.h>

#if 0
    #define Verbose(msg, ...) (void)0
    #define Info(msg, ...) (void)0
    #define Trace(msg, ...) (void)0
    #define Warn(msg, ...) (void)0
    #define Error(msg, ...) (void)0
#else
    #define Verbose(msg, ...) printf("[verb] [%s] ", __FUNCTION__);printf(msg, ##__VA_ARGS__);printf("\n")
//    #define Info(msg, ...) (void)0
    #define Info(msg, ...) printf("[info] ");printf(msg, ##__VA_ARGS__);printf("\n")
    #define Trace(msg, ...) printf("[trace] ");printf(msg, ##__VA_ARGS__);printf("\n")
    #define Warn(msg, ...) printf("[warn] ");printf(msg, ##__VA_ARGS__);printf("\n")
    #define Error(msg, ...) printf("[error] ");printf(msg, ##__VA_ARGS__);printf("\n")
#endif

#endif