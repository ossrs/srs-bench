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

#ifndef _htl_app_http_client_hpp
#define _htl_app_http_client_hpp

/*
#include <htl_app_http_client.hpp>
*/
#include <string>

#include <htl_core_uri.hpp>
#include <htl_os_st.hpp>

class StHttpClient
{
private:
    std::string connected_ip;
    HttpUrl* connected_url;
    StSocket* socket;
private:
    http_parser http_header;
public:
    StHttpClient();
    virtual ~StHttpClient();
public:
    /**
    * download the content specified by url using HTTP GET method.
    * @response the string pointer which store the content. ignore content if set to NULL.
    */
    virtual int DownloadString(HttpUrl* url, std::string* response);
public:
    /**
    * when get response and parse header completed, return the header.
    * if not parsed, header set to zero.
    */
    virtual http_parser* GetResponseHeader();
private:
    static int on_headers_complete(http_parser* parser);
    virtual void OnHeaderCompleted(http_parser* parser);
private:
    virtual int ParseResponse(HttpUrl* url, string* response);
    virtual int ParseResponseBody(HttpUrl* url, string* response, int body_received);
    virtual int ParseResponseBodyData(HttpUrl* url, string* response, size_t body_left, const void* buf, size_t size);
    virtual int ParseResponseHeader(string* response, int& body_received);
    virtual int Connect(HttpUrl* url);
    virtual int CheckUrl(HttpUrl* url);
};

#endif
