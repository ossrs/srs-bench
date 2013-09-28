#ifndef _htl_app_rtmp_load_hpp
#define _htl_app_rtmp_load_hpp

/*
#include <htl_app_rtmp_load.hpp>
*/
#include <htl_app_task_base.hpp>

// for rtmp task.
class StRtmpTask : public StBaseTask
{
private:
    HttpUrl url;
public:
    StRtmpTask();
    virtual ~StRtmpTask();
public:
    virtual int Initialize(std::string http_url, double startup, double delay, double error, int count);
protected:
    virtual Uri* GetUri();
    virtual int ProcessTask();
};

#endif