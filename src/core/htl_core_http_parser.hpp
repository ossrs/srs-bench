#ifndef _htl_core_http_parser_hpp
#define _htl_core_http_parser_hpp

/*
#include <htl_core_http_parser.hpp>
*/

#include <string>

#include <http_parser.h>

class HttpParser
{
};

class HttpUri
{
private:
    std::string url;
    http_parser_url hp_u;
private:
    std::string protocol;
    std::string host;
    int port;
    std::string path;
public:
    HttpUri();
    virtual ~HttpUri();
public:
    virtual int Initialize(std::string http_url);
public:
    virtual const char* GetProtocol();
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

#endif
