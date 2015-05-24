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

#ifndef _htl_app_task_base_hpp
#define _htl_app_task_base_hpp

/*
#include <htl_app_task_base.hpp>
*/
#include <string>

#include <http_parser.h>
#include <htl_core_uri.hpp>

#include <htl_os_st.hpp>

class StBaseTask : public StTask
{
protected:
    double startup_seconds;
    double delay_seconds;
    double error_seconds;
    int count;
public:
    StBaseTask();
    virtual ~StBaseTask();
protected:
    virtual int InitializeBase(std::string http_url, double startup, double delay, double error, int count);
public:
    virtual int Process();
protected:
    virtual Uri* GetUri() = 0;
    virtual int ProcessTask() = 0;
};

#endif
