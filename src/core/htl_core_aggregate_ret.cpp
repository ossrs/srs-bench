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

