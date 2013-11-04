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

