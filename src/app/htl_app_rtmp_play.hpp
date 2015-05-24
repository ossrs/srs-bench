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

#ifndef _htl_app_rtmp_play_hpp
#define _htl_app_rtmp_play_hpp

/*
#include <htl_app_rtmp_play.hpp>
*/
#include <string>

#include <htl_core_uri.hpp>
#include <htl_os_st.hpp>

#include <htl_app_rtmp_protocol.hpp>

class StRtmpPlayClient
{
protected:
    srs_rtmp_t srs;
    int stream_id;
public:
    StRtmpPlayClient();
    virtual ~StRtmpPlayClient();
public:
    virtual int Dump(RtmpUrl* url);
protected:
    virtual int Connect(RtmpUrl* url);
    virtual int Handshake();
    virtual int ConnectApp();
    virtual int PlayStram();
    virtual int DumpAV();
};

class StRtmpPlayClientFast : public StRtmpPlayClient
{
public:
    StRtmpPlayClientFast();
    virtual ~StRtmpPlayClientFast();
protected:
    virtual int DumpAV();
};

#endif
