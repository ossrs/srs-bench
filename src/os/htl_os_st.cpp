#include <htl_core_error.hpp>
#include <htl_core_log.hpp>

#include <htl_os_st.hpp>

StTask::StTask(){
    static int _id = 0;
    id = ++_id;
}

StTask::~StTask(){
}

int StTask::GetId(){
    return id;
}

StFarm::StFarm(){
}

StFarm::~StFarm(){
}

int StFarm::Initialize(){
    int ret = ERROR_SUCCESS;
    
    // use linux epoll.
    if(st_set_eventsys(ST_EVENTSYS_ALT) == -1){
        ret = ERROR_ST_INITIALIZE;
        Error("st_set_eventsys use linux epoll failed. ret=%d", ret);
        return ret;
    }
    
    if(st_init() != 0){
        ret = ERROR_ST_INITIALIZE;
        Error("st_init failed. ret=%d", ret);
        return ret;
    }
    
    return ret;
}

int StFarm::Spawn(StTask* task){
    int ret = ERROR_SUCCESS;
    
    if(st_thread_create(st_thread_function, task, 0, 0) == NULL){
        ret = ERROR_ST_THREAD_CREATE;
        Error("crate st_thread failed, ret=%d", ret);
        return ret;
    }
    
    Trace("create thread for task #%d success", task->GetId());
    
    return ret;
}

int StFarm::WaitAll(){
    int ret = ERROR_SUCCESS;
    
    st_thread_exit(NULL);
    
    return ret;
}

void* StFarm::st_thread_function(void* args){
    StTask* task = (StTask*)args;
    
    context->SetId(task->GetId());
    
    int ret = task->Process();
    
    if(ret != ERROR_SUCCESS){
        Warn("st task terminate with ret=%d", ret);
    }
    else{
        Trace("st task terminate with ret=%d", ret);
    }

    delete task;
    
    return NULL;
}

StLogContext::StLogContext(){
}

StLogContext::~StLogContext(){
}

void StLogContext::SetId(int id){
    cache[st_thread_self()] = id;
}

int StLogContext::GetId(){
    return cache[st_thread_self()];
}
