#ifndef _htl_app_http_load_hpp
#define _htl_app_http_load_hpp

/*
#include <htl_app_http_load.hpp>
*/
#include <htl_app_task_base.hpp>

// for http task.
class StHttpTask : public StBaseTask
{
private:
    HttpUrl url;
public:
    StHttpTask();
    virtual ~StHttpTask();
public:
    virtual int Initialize(std::string http_url, double startup, double delay, double error, int count);
protected:
    virtual Uri* GetUri();
    virtual int ProcessTask();
};

#endif