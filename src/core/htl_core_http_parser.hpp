#ifndef _htl_core_http_parser_hpp
#define _htl_core_http_parser_hpp

/*
#include <htl_core_http_parser.hpp>
*/

#include <string>

#include <http_parser.h>

class HttpUrl
{
private:
    std::string url;
    http_parser_url hp_u;
private:
    std::string schema;
    std::string host;
    int port;
    std::string path;
public:
    HttpUrl();
    virtual ~HttpUrl();
public:
    virtual int Initialize(std::string http_url);
public:
    virtual const char* GetUrl();
    virtual HttpUrl* Copy();
public:
    virtual const char* GetSchema();
    virtual const char* GetHost();
    virtual int GetPort();
    virtual const char* GetPath();
private:
    virtual std::string Get(http_parser_url_fields field);
    /**
    * get the parsed url field.
    * @return return empty string if not set.
    */
    static std::string GetUriField(std::string uri, http_parser_url* hp_u, http_parser_url_fields field);
};

class HttpParser
{
public:
    HttpParser();
    virtual ~HttpParser();
public:
};

#endif
