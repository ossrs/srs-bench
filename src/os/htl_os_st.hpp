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

#ifndef _htl_os_st_hpp
#define _htl_os_st_hpp

/*
#include <htl_os_st.hpp>
*/

#include <map>
#include <string>

#include <st.h>

#include <htl_core_log.hpp>

// the statistic for each st-thread(task)
class StStatistic
{
private:
    int threads, alive;
    int64_t starttime, task_duration;
    int64_t nread, nwrite;
    int64_t tasks, err_tasks, sub_tasks, err_sub_tasks;
public:
    StStatistic();
    virtual ~StStatistic();
public:
    virtual void OnRead(int tid, ssize_t nread_bytes);
    virtual void OnWrite(int tid, ssize_t nwrite_bytes);
public:
    // when task thread run.
    virtual void OnThreadRun(int tid);
    // when task thread quit.
    virtual void OnThreadQuit(int tid);
    // when task start request url, ie. get m3u8
    virtual void OnTaskStart(int tid, std::string task_url);
    // when task error.
    virtual void OnTaskError(int tid, int duration_seconds);
    // when task finish request url, ie. finish all ts in m3u8
    virtual void OnTaskEnd(int tid, int duration_seconds);
    // when sub task start, ie. get ts in m3u8
    virtual void OnSubTaskStart(int tid, std::string sub_task_url);
    // when sub task error.
    virtual void OnSubTaskError(int tid, int duration_seconds);
    // when sub task finish, ie. finish a ts.
    virtual void OnSubTaskEnd(int tid, int duration_seconds);
public:
    virtual void DoReport(double sleep_ms);
};

extern StStatistic* statistic;

// abstract task for st, which run in a st-thread.
class StTask
{
private:
    int id;
public:
    StTask();
    virtual ~StTask();
public:
    virtual int GetId();
public:
    /**
    * the framework will start a thread for the task, 
    * then invoke the Process function to do actual transaction.
    */
    virtual int Process() = 0;
};

// the farm for all StTask, to spawn st-thread and process all task.
class StFarm
{
private:
    double report_seconds;
public:
    StFarm();
    virtual ~StFarm();
public:
    virtual int Initialize(double report);
    virtual int Spawn(StTask* task);
    virtual int WaitAll();
private:
    static void* st_thread_function(void* args);
};

// the socket status
enum SocketStatus{
    SocketInit,
    SocketConnected,
    SocketDisconnected,
};

// the socket base on st.
class StSocket
{
private:
    SocketStatus status;
    st_netfd_t sock_nfd;
public:
    StSocket();
    virtual ~StSocket();
public:
    virtual st_netfd_t GetStfd();
    virtual SocketStatus Status();
    virtual int Connect(const char* ip, int port);
    virtual int Read(const void* buf, size_t size, ssize_t* nread);
    virtual int Readv(const iovec *iov, int iov_size, ssize_t* nread);
    virtual int ReadFully(const void* buf, size_t size, ssize_t* nread);
    virtual int Write(const void* buf, size_t size, ssize_t* nwrite);
    virtual int Writev(const iovec *iov, int iov_size, ssize_t* nwrite);
    virtual int Close();
};

// common utilities.
class StUtility
{
public:
    static int64_t GetCurrentTime();
    static void InitRandom();
    static st_utime_t BuildRandomMTime(double sleep_seconds);
    static int DnsResolve(std::string host, std::string& ip);
};

// st-thread based log context.
class StLogContext : public LogContext
{
private:
    std::map<st_thread_t, int> cache;
public:
    StLogContext();
    virtual ~StLogContext();
public:
    virtual void SetId(int id);
    virtual int GetId();
};

#endif
