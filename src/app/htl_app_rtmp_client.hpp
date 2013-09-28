#ifndef _htl_app_rtmp_client_hpp
#define _htl_app_rtmp_client_hpp

/*
#include <htl_app_rtmp_client.hpp>
*/
#include <string>

#include <htl_core_uri.hpp>
#include <htl_os_st.hpp>

// max client message size, the message client sent to server.
#define RTMP_CLINET_MSG_MAX_SIZE 8192
class Rtmp
{
private:
    int in_chunk_size;
    int out_chunk_size;
private:
    char buffer[RTMP_CLINET_MSG_MAX_SIZE];
    char* p_chunk;
    int chunk_fragments;
    char* p;
    int header_size;
// protocol info
private:
    int ack_size;
    int64_t acked_size;
    int64_t recv_size;
// chunk info
private:
    char cid;
    bool extended_timestamp;
// message info
private:
    int32_t timestamp;
    int32_t message_length;
    char message_type;
    int32_t message_stream_id;
public:
    Rtmp();
    virtual ~Rtmp();
public:
    virtual int GetSize();
    virtual const char* GetBuffer();
public:
    virtual char GetMessageType();
    virtual int GetBodySize();
    virtual const char* GetBody();
public:
    virtual int WriteBegin(char cid, int32_t timestamp, char message_type, int32_t message_stream_id);
    virtual int WriteAMF0String(const char* value);
    virtual int WriteAMF0Boolean(bool value);
    virtual int WriteAMF0Number(double value);
    virtual int WriteAMF0ObjectStart();
    virtual int WriteAMF0ObjectPropertyName(const char* value);
    virtual int WriteAMF0ObjectEnd();
    virtual int WriteInt32(int32_t value);
    virtual int WriteEnd();
public:
    virtual int Read(StSocket* socket);
private:
    virtual int ParseHeader(StSocket* socket);
    virtual int FilterPacket(StSocket* socket);
};

class StRtmpClient
{
private:
    Rtmp rtmp;
private:
    StSocket* socket;
public:
    StRtmpClient();
    virtual ~StRtmpClient();
public:
    virtual int Dump(RtmpUrl* url);
private:
    virtual int Connect(RtmpUrl* url);
    virtual int Handshake();
    virtual int ConnectApp(RtmpUrl* url);
    virtual int PlayStram(RtmpUrl* url);
    virtual int DumpAV();
};

#endif