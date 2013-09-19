#ifndef _htl_app_http_load_hpp
#define _htl_app_http_load_hpp

/*
#include <htl_app_http_load.hpp>
*/

#include <htl_os_st.hpp>

class StHttpTask : public StTask
{
public:
    StHttpTask();
    virtual ~StHttpTask();
public:
    virtual int Process();
};

#endif