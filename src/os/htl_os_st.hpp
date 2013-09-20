#ifndef _htl_os_st_hpp
#define _htl_os_st_hpp

/*
#include <htl_os_st.hpp>
*/

#include <map>

#include <st.h>

#include <htl_core_log.hpp>

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
public:
    StFarm();
    virtual ~StFarm();
public:
    virtual int Initialize();
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
    virtual SocketStatus Status();
    virtual int Connect(const char* ip, int port);
    virtual int Read(const void* buf, size_t size, ssize_t* nread);
    virtual int Write(const void* buf, size_t size, ssize_t* nwrite);
    virtual int Close();
};

// common utilities.
class StUtility
{
public:
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