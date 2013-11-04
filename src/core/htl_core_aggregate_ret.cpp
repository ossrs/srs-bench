#include <htl_stdinc.hpp>

#include <vector>
using namespace std;

#include <htl_core_error.hpp>

#include <htl_core_aggregate_ret.hpp>

AggregateRet::AggregateRet(){
}

AggregateRet::~AggregateRet(){
}

void AggregateRet::Add(int ret){
    rets.push_back(ret);
}

int AggregateRet::GetReturnValue(){
    int ret = ERROR_SUCCESS;
    
    for(vector<int>::iterator ite = rets.begin(); ite != rets.end(); ++ite){
        int item_ret = *ite;
        
        if((ret = item_ret) != ERROR_SUCCESS){
            return ret;
        }
    }
    
    return ret;
}

