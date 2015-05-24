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

#ifndef _htl_core_error_hpp
#define _htl_core_error_hpp

/*
#include <htl_core_error.hpp>
*/

#define ERROR_SUCCESS 0

#define ERROR_SOCKET 100
#define ERROR_OPEN_SOCKET 101
#define ERROR_CONNECT 102
#define ERROR_SEND 103
#define ERROR_READ 104
#define ERROR_CLOSE 105
#define ERROR_DNS_RESOLVE 106

#define ERROR_URL_INVALID 200
#define ERROR_HTTP_RESPONSE 201
#define ERROR_HLS_INVALID 202

#define ERROR_NOT_SUPPORT 300

#define ERROR_ST_INITIALIZE 400
#define ERROR_ST_THREAD_CREATE 401

#define ERROR_HP_PARSE_URL 500
#define ERROR_HP_EP_CHNAGED 501
#define ERROR_HP_PARSE_RESPONSE 502

#define ERROR_RTMP_URL 600
#define ERROR_RTMP_OVERFLOW 601
#define ERROR_RTMP_MSG_TOO_BIG 602
#define ERROR_RTMP_INVALID_RESPONSE 603
#define ERROR_RTMP_OPEN_FLV 604

#define ProductVersion "1.0.14"
#define ProductHTTPName "SB(SRS Bench) HttpLoad/"ProductVersion
#define ProductHLSName "SB(SRS Bench) HlsLoad/"ProductVersion
#define ProductRtmpName "SB(SRS Bench) RtmpPlayLoad/"ProductVersion
#define ProductRtmpPublishName "SB(SRS Bench) RtmpPublishLoad/"ProductVersion
#define BuildPlatform "linux"
#define BugReportEmail "winlin@vip.126.com"

#define HTTP_HEADER_BUFFER 1024
#define HTTP_BODY_BUFFER 32*1024

#endif

