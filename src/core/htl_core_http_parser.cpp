#include <stdlib.h>

#include <string>
using namespace std;

#include <htl_core_log.hpp>
#include <htl_core_error.hpp>

#include <htl_core_http_parser.hpp>

#define PROTOCOL_HTTP "http://"
#define PROTOCOL_HTTPS "https://"

HttpUri::HttpUri(){
    protocol = PROTOCOL_HTTP;
    port = 80;
}

HttpUri::~HttpUri(){
}

int HttpUri::Initialize(std::string http_url){
    int ret = ERROR_SUCCESS;
    
    url = http_url;
    const char* purl = url.c_str();

    if((ret = http_parser_parse_url(purl, url.length(), 0, &hp_u)) != 0){
        int code = ret;
        ret = ERROR_HP_PARSE_URL;
        
        Error("parse url %s failed, code=%d, ret=%d", purl, code, ret);
        return ret;
    }
    
    if(Get(UF_SCHEMA) != ""){
        protocol = Get(UF_SCHEMA);
    }
    
    host = Get(UF_HOST);
    
    if(Get(UF_PORT) != ""){
        port = atoi(Get(UF_PORT).c_str());
    }
    
    path = Get(UF_PATH);
    
    return ret;
}

const char* HttpUri::GetProtocol(){
    return protocol.c_str();
}

const char* HttpUri::GetHost(){
    return host.c_str();
}

int HttpUri::GetPort(){
    return port;
}

const char* HttpUri::GetPath(){
    return path.c_str();
}

string HttpUri::Get(http_parser_url_fields field){
    return HttpUri::GetUriField(url, &hp_u, field);
}

string HttpUri::GetUriField(string uri, http_parser_url* hp_u, http_parser_url_fields field){
    if((hp_u->field_set & (1 << field)) == 0){
        return "";
    }
    
    Verbose("uri field matched, off=%d, len=%d, value=%.*s", 
        hp_u->field_data[field].off, hp_u->field_data[field].len, hp_u->field_data[field].len, 
        uri.c_str() + hp_u->field_data[field].off);
        
    return uri.substr(hp_u->field_data[field].off, hp_u->field_data[field].len);
}
