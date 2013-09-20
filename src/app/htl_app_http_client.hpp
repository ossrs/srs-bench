#ifndef _htl_app_http_client_hpp
#define _htl_app_http_client_hpp

/*
#include <htl_app_http_client.hpp>
*/
#include <string>

#include <htl_core_http_parser.hpp>
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
    virtual int ParseResponseHeader(string* response, int& body_received);
    virtual int Connect(HttpUrl* url);
    virtual int CheckUrl(HttpUrl* url);
};

#endif