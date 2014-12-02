#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <assert.h>

#include <string>
#include <sstream>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>

#include <htl_app_rtmp_client.hpp>
#include <htl_app_rtmp_protocol.hpp>

#ifdef SRS_HIJACK_IO
srs_hijack_io_t srs_hijack_io_create()
{
    return new StSocket();
}

void srs_hijack_io_destroy(srs_hijack_io_t ctx)
{
    StSocket* skt = (StSocket*)ctx;
    delete skt;
}

int srs_hijack_io_create_socket(srs_hijack_io_t /*ctx*/)
{
    return ERROR_SUCCESS;
}

int srs_hijack_io_connect(srs_hijack_io_t ctx, const char* server_ip, int port)
{
    StSocket* skt = (StSocket*)ctx;
    return skt->Connect(server_ip, port);
}

int srs_hijack_io_read(srs_hijack_io_t ctx, void* buf, size_t size, ssize_t* nread)
{
    StSocket* skt = (StSocket*)ctx;
    return skt->Read(buf, size, nread);
}

void srs_hijack_io_set_recv_timeout(srs_hijack_io_t /*ctx*/, int64_t /*timeout_us*/)
{
}

int64_t srs_hijack_io_get_recv_timeout(srs_hijack_io_t /*ctx*/)
{
    return ST_UTIME_NO_TIMEOUT;
}

int64_t srs_hijack_io_get_recv_bytes(srs_hijack_io_t /*ctx*/)
{
    return 0;
}

void srs_hijack_io_set_send_timeout(srs_hijack_io_t /*ctx*/, int64_t /*timeout_us*/)
{
}

int64_t srs_hijack_io_get_send_timeout(srs_hijack_io_t /*ctx*/)
{
    return ST_UTIME_NO_TIMEOUT;
}

int64_t srs_hijack_io_get_send_bytes(srs_hijack_io_t /*ctx*/)
{
    return 0;
}

int srs_hijack_io_writev(srs_hijack_io_t ctx, const iovec *iov, int iov_size, ssize_t* nwrite)
{
    StSocket* skt = (StSocket*)ctx;
    return skt->Writev(iov, iov_size, nwrite);
}

bool srs_hijack_io_is_never_timeout(srs_hijack_io_t /*ctx*/, int64_t /*timeout_us*/)
{
    return true;
}

int srs_hijack_io_read_fully(srs_hijack_io_t ctx, void* buf, size_t size, ssize_t* nread)
{
    StSocket* skt = (StSocket*)ctx;
    return skt->ReadFully(buf, size, nread);
}

int srs_hijack_io_write(srs_hijack_io_t ctx, void* buf, size_t size, ssize_t* nwrite)
{
    StSocket* skt = (StSocket*)ctx;
    return skt->Write(buf, size, nwrite);
}
#endif

StRtmpClient::StRtmpClient(){
    stream_id = 0;
    srs = NULL;
}

StRtmpClient::~StRtmpClient(){
    srs_rtmp_destroy(srs);
}

int StRtmpClient::Dump(RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    
    if((ret = Connect(url)) != ERROR_SUCCESS){
        Error("rtmp client connect server failed. ret=%d", ret);
        return ret;
    }
    
    if((ret = Handshake()) != ERROR_SUCCESS){
        Error("rtmp client handshake failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client handshake success");
    
    if((ret = ConnectApp()) != ERROR_SUCCESS){
        Error("rtmp client connect tcUrl failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client connect tcUrl(%s) success", url->GetTcUrl());
    
    if((ret = PlayStram()) != ERROR_SUCCESS){
        Error("rtmp client play stream failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client play stream(%s) success", url->GetUrl());
    
    if((ret = DumpAV()) != ERROR_SUCCESS){
        Error("rtmp client dump av failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client dump av success");

    return ret;
}

int StRtmpClient::Connect(RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    
    StSocket* socket = (StSocket*)srs_hijack_io_get(srs);
    
    if(socket && socket->Status() == SocketConnected){
        return ret;
    }
    
    srs_rtmp_destroy(srs);
    srs = srs_rtmp_create(url->GetUrl());

    if ((ret = __srs_rtmp_dns_resolve(srs)) != ERROR_SUCCESS){
        Error("dns resolve failed. ret=%d", ret);
        return ret;
    }
    
    if ((ret = __srs_rtmp_connect_server(srs)) != ERROR_SUCCESS){
        Error("connect to server failed. ret=%d", ret);
        return ret;
    }
    
    Info("socket connected on url %s", url->GetUrl());
    
    return ret;
}

int StRtmpClient::Handshake(){
    return __srs_rtmp_do_simple_handshake(srs);
}

int StRtmpClient::ConnectApp(){
    return srs_rtmp_connect_app(srs);
}

int StRtmpClient::PlayStram(){
    return srs_rtmp_play_stream(srs);
}

int StRtmpClient::DumpAV(){
    int ret = ERROR_SUCCESS;

    // recv response
    while(true){
        char type;
        u_int32_t timestamp;
        char* data;
        int size;
        
        if ((ret = srs_rtmp_read_packet(srs, &type, &timestamp, &data, &size)) != ERROR_SUCCESS){
            return ret;
        }
        
        Info("get message type=%d, size=%d", type, size);
        delete data;
    }
    
    return ret;
}
