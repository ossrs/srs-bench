#ifndef _htl_app_http_load_hpp
#define _htl_app_http_load_hpp

/*
#include <htl_app_http_load.hpp>
*/
#include <string>

#include <http_parser.h>

#include <htl_core_http_parser.hpp>

#include <htl_os_st.hpp>

// for http task.
class StHttpTask : public StTask
{
private:
    HttpUri url;
public:
    StHttpTask();
    virtual ~StHttpTask();
public:
    virtual int Initialize(std::string http_url);
    virtual int Process();
};

#endif