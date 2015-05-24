/*
The MIT License (MIT)

Copyright (c) 2013-2015 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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
    RtmpUrl url;
public:
    StRtmpTask();
    virtual ~StRtmpTask();
public:
    virtual int Initialize(std::string http_url, double startup, double delay, double error, int count);
protected:
    virtual Uri* GetUri();
    virtual int ProcessTask();
};

// for rtmp task with fast algorihtm.
// donot recv in RTMP, but directly in TCP.
class StRtmpTaskFast : public StBaseTask
{
private:
    RtmpUrl url;
public:
    StRtmpTaskFast();
    virtual ~StRtmpTaskFast();
public:
    virtual int Initialize(std::string http_url, double startup, double delay, double error, int count);
protected:
    virtual Uri* GetUri();
    virtual int ProcessTask();
};

// for rtmp publish load task.
class StRtmpPublishTask : public StBaseTask
{
private:
    std::string input_flv_file;
    RtmpUrl url;
public:
    StRtmpPublishTask();
    virtual ~StRtmpPublishTask();
public:
    virtual int Initialize(std::string input, 
        std::string http_url, double startup, double delay, double error, int count);
protected:
    virtual Uri* GetUri();
    virtual int ProcessTask();
};

#endif
