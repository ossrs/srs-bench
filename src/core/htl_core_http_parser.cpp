#include <htl_stdinc.hpp>

#include <stdlib.h>

#include <string>
using namespace std;

#include <htl_core_log.hpp>
#include <htl_core_error.hpp>

#include <htl_core_http_parser.hpp>

HttpUrl::HttpUrl(){
    port = 80;
}

HttpUrl::~HttpUrl(){
}

int HttpUrl::Initialize(std::string http_url){
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
        schema = Get(UF_SCHEMA);
    }
    
    host = Get(UF_HOST);
    
    if(Get(UF_PORT) != ""){
        port = atoi(Get(UF_PORT).c_str());
    }
    
    path = Get(UF_PATH);
    
    return ret;
}

const char* HttpUrl::GetUrl(){
    return url.c_str();
}

HttpUrl* HttpUrl::Copy(){
    HttpUrl* copy = new HttpUrl();
    
    copy->Initialize(url);
    
    return copy;
}

const char* HttpUrl::GetSchema(){
    return schema.c_str();
}

const char* HttpUrl::GetHost(){
    return host.c_str();
}

int HttpUrl::GetPort(){
    return port;
}

const char* HttpUrl::GetPath(){
    return path.c_str();
}

string HttpUrl::Get(http_parser_url_fields field){
    return HttpUrl::GetUriField(url, &hp_u, field);
}

string HttpUrl::GetUriField(string uri, http_parser_url* hp_u, http_parser_url_fields field){
    if((hp_u->field_set & (1 << field)) == 0){
        return "";
    }
    
    Verbose("uri field matched, off=%d, len=%d, value=%.*s", 
        hp_u->field_data[field].off, hp_u->field_data[field].len, hp_u->field_data[field].len, 
        uri.c_str() + hp_u->field_data[field].off);
        
    return uri.substr(hp_u->field_data[field].off, hp_u->field_data[field].len);
}

HttpParser::HttpParser(){
}

HttpParser::~HttpParser(){
}
