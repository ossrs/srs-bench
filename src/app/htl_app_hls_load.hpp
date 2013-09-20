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
#include <htl_app_m3u8_parser.hpp>

// for http task.
class StHlsTask : public StTask
{
private:
    // the last downloaded ts url to prevent download multile times.
    M3u8TS last_downloaded_ts;
    int target_duration;
private:
    HttpUrl url;
    double startup_seconds;
    double delay_seconds;
    double error_seconds;
    int count;
public:
    StHlsTask();
    virtual ~StHlsTask();
public:
    virtual int Initialize(std::string http_url, double startup, double delay, double error, int count);
    virtual int Process();
private:
    virtual int ProcessM3u8(StHttpClient& client);
    virtual int ProcessTS(StHttpClient& client, std::vector<M3u8TS>& ts_objects);
    virtual int DownloadTS(StHttpClient& client, M3u8TS& ts);
};

#endif