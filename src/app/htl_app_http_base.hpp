#ifndef _htl_app_http_base_hpp
#define _htl_app_http_base_hpp

/*
#include <htl_app_http_base.hpp>
*/
#include <string>

#include <http_parser.h>

#include <htl_core_http_parser.hpp>

#include <htl_os_st.hpp>

class StHttpBaseTask : public StTask
{
protected:
    HttpUrl url;
    double startup_seconds;
    double delay_seconds;
    double error_seconds;
    int count;
public:
    StHttpBaseTask();
    virtual ~StHttpBaseTask();
public:
    virtual int Initialize(std::string http_url, double startup, double delay, double error, int count);
    virtual int Process();
protected:
    virtual int ProcessHttp() = 0;
};

#endif