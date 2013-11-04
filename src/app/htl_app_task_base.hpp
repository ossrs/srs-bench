#ifndef _htl_app_task_base_hpp
#define _htl_app_task_base_hpp

/*
#include <htl_app_task_base.hpp>
*/
#include <string>

#include <http_parser.h>
#include <htl_core_uri.hpp>

#include <htl_os_st.hpp>

class StBaseTask : public StTask
{
protected:
    double startup_seconds;
    double delay_seconds;
    double error_seconds;
    int count;
public:
    StBaseTask();
    virtual ~StBaseTask();
protected:
    virtual int InitializeBase(std::string http_url, double startup, double delay, double error, int count);
public:
    virtual int Process();
protected:
    virtual Uri* GetUri() = 0;
    virtual int ProcessTask() = 0;
};

#endif
