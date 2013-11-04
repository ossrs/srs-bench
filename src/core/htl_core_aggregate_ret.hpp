#ifndef _htl_core_aggregate_ret_hpp
#define _htl_core_aggregate_ret_hpp

/*
#include <htl_core_aggregate_ret.hpp>
*/
#include <vector>

class AggregateRet
{
private:
    std::vector<int> rets;
public:
    AggregateRet();
    virtual ~AggregateRet();
public:
    /**
    * add return value to collection.
    */
    virtual void Add(int ret);
    /**
    * get the summary of return value, if any failed, return it.
    */
    virtual int GetReturnValue();
};
    
#endif
