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

#ifndef _htl_app_m3u8_parser_hpp
#define _htl_app_m3u8_parser_hpp

/*
#include <htl_app_m3u8_parser.hpp>
*/
#include <string>
#include <vector>

#include <htl_core_uri.hpp>

struct M3u8TS
{
    std::string ts_url;
    double duration;
    
    bool operator== (const M3u8TS& b)const{
        return ts_url == b.ts_url;
    }
};

class HlsM3u8Parser
{
public:
    HlsM3u8Parser();
    virtual ~HlsM3u8Parser();
public:
    static int ParseM3u8Data(HttpUrl* url, std::string m3u8, std::vector<M3u8TS>& ts_objects, int& target_duration, std::string& variant);
};

#endif
