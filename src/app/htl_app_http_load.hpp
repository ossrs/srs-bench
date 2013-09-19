#ifndef _htl_app_http_load_hpp
#define _htl_app_http_load_hpp

/*
#include <htl_app_http_load.hpp>
*/
#include <string>

#include <htl_os_st.hpp>

// for http task.
class StHttpTask : public StTask
{
private:
    std::string url;
public:
    StHttpTask(std::string http_url);
    virtual ~StHttpTask();
public:
    virtual int Process();
};

#endif