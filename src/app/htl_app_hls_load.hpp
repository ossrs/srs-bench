#ifndef _htl_app_hls_load_hpp
#define _htl_app_hls_load_hpp

/*
#include <htl_app_hls_load.hpp>
*/
#include <htl_app_http_client.hpp>
#include <htl_app_m3u8_parser.hpp>
#include <htl_app_http_base.hpp>

// for http task.
class StHlsTask : public StHttpBaseTask
{
private:
    // the last downloaded ts url to prevent download multile times.
    M3u8TS last_downloaded_ts;
    int target_duration;
public:
    StHlsTask();
    virtual ~StHlsTask();
protected:
    virtual int ProcessHttp();
private:
    virtual int ProcessM3u8(StHttpClient& client);
    virtual int ProcessTS(StHttpClient& client, std::vector<M3u8TS>& ts_objects);
    virtual int DownloadTS(StHttpClient& client, M3u8TS& ts);
};

#endif