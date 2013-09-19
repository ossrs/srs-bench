#ifndef _htl_os_st_hpp
#define _htl_os_st_hpp

/*
#include <htl_os_st.hpp>
*/

#include <st.h>

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

#endif