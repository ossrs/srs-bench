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

#ifndef _htl_core_uri_hpp
#define _htl_core_uri_hpp

/*
#include <htl_core_uri.hpp>
*/

#include <string>

#include <http_parser.h>

class Uri
{
public:
    Uri();
    virtual ~Uri();
public:
    virtual int Initialize(std::string http_url) = 0;
};

class ProtocolUrl : public Uri
{
protected:
    std::string url;
    http_parser_url hp_u;
protected:
    std::string schema;
    std::string host;
    int port;
    std::string path;
    std::string query;
public:
    ProtocolUrl();
    virtual ~ProtocolUrl();
public:
    virtual int Initialize(std::string http_url);
public:
    virtual const char* GetUrl();
    virtual const char* GetSchema();
    virtual const char* GetHost();
    virtual int GetPort();
protected:
    virtual std::string Get(http_parser_url_fields field);
    /**
    * get the parsed url field.
    * @return return empty string if not set.
    */
    static std::string GetUriField(std::string uri, http_parser_url* hp_u, http_parser_url_fields field);
};

class HttpUrl : public ProtocolUrl
{
private:
    std::string path_query;
public:
    HttpUrl();
    virtual ~HttpUrl();
public:
    virtual std::string Resolve(std::string origin_url);
    virtual HttpUrl* Copy();
public:
    virtual const char* GetPath();
};

class RtmpUrl : public ProtocolUrl
{
private:
    std::string tcUrl;
    std::string vhost;
    std::string app;
    std::string stream;
public:
    RtmpUrl();
    virtual ~RtmpUrl();
public:
    virtual int Initialize(std::string http_url);
public:
    virtual const char* GetTcUrl();
    virtual const char* GetVhost();
    virtual const char* GetApp();
    virtual const char* GetStream();
};

#endif

