#ifndef _htl_app_hls_load_hpp
#define _htl_app_hls_load_hpp

/*
#include <htl_app_hls_load.hpp>
*/
#include <string>
#include <vector>

#include <http_parser.h>

#include <htl_core_http_parser.hpp>
#include <htl_app_http_client.hpp>

#include <htl_os_st.hpp>

struct M3u8TS
{
    string ts_url;
    double duration;
    
    bool operator== (const M3u8TS& b)const{
        return ts_url == b.ts_url;
    }
};

// for http task.
class StHlsTask : public StTask
{
private:
    // the last downloaded ts url to prevent download multile times.
    M3u8TS last_downloaded_ts;
    int target_duration;
private:
    HttpUrl url;
    double delay_seconds;
    double error_seconds;
    int count;
public:
    StHlsTask();
    virtual ~StHlsTask();
public:
    virtual int Initialize(std::string http_url, double delay, double error, int count);
    virtual int Process();
private:
    virtual int ProcessM3u8(StHttpClient& client);
    virtual int ProcessTS(StHttpClient& client, std::vector<M3u8TS>& ts_objects);
    virtual int DownloadTS(StHttpClient& client, M3u8TS& ts);
    virtual int ParseM3u8Data(std::string m3u8, std::vector<M3u8TS>& ts_objects);
};

#endif