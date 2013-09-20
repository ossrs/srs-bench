#ifndef _htl_app_m3u8_parser_hpp
#define _htl_app_m3u8_parser_hpp

/*
#include <htl_app_m3u8_parser.hpp>
*/
#include <string>
#include <vector>

struct M3u8TS
{
    std::string ts_url;
    double duration;
    
    bool operator== (const M3u8TS& b)const{
        return ts_url == b.ts_url;
    }
};

class HlsM3u8Parser
{
public:
    HlsM3u8Parser();
    virtual ~HlsM3u8Parser();
public:
    static int ParseM3u8Data(HttpUrl* url, std::string m3u8, std::vector<M3u8TS>& ts_objects, int& target_duration);
};

#endif