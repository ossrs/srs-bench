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

#ifndef _htl_app_hls_load_hpp
#define _htl_app_hls_load_hpp

/*
#include <htl_app_hls_load.hpp>
*/
#include <htl_app_http_client.hpp>
#include <htl_app_m3u8_parser.hpp>
#include <htl_app_task_base.hpp>

// for http task.
class StHlsTask : public StBaseTask
{
private:
    HttpUrl url;
    // the last downloaded ts url to prevent download multile times.
    M3u8TS last_downloaded_ts;
    int target_duration;
    bool is_vod;
public:
    StHlsTask();
    virtual ~StHlsTask();
public:
    virtual int Initialize(std::string http_url, bool vod, double startup, double delay, double error, int count);
protected:
    virtual Uri* GetUri();
    virtual int ProcessTask();
private:
    virtual int ProcessM3u8(StHttpClient& client);
    virtual int ProcessTS(StHttpClient& client, std::vector<M3u8TS>& ts_objects);
    virtual int DownloadTS(StHttpClient& client, M3u8TS& ts);
};

#endif
