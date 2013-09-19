#include <htl_core_error.hpp>

#include <htl_os_st.hpp>

StTask::StTask(){
}

StTask::~StTask(){
}

StFarm::StFarm(){
}

StFarm::~StFarm(){
}
int StFarm::Spawn(StTask* task){
    int ret = ERROR_SUCCESS;
    return ret;
}

int StFarm::WaitAll(){
    int ret = ERROR_SUCCESS;
    return ret;
}
