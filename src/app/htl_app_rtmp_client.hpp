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
#define RTMP_CLIENT_PCUC_MAX_SIZE 64
#define RTMP_MAX_CHUNK_STREAMS 10000

struct ChunkStream
{
public:
    int in_chunk_size;
    int out_chunk_size;
// chunk info
public:
    char fmt;
    char cid;
    bool extended_timestamp;
// message info
public:
    int32_t timestamp;
    int32_t message_length;
    char message_type;
    int32_t message_stream_id;
// buffer
public:
    char* buffer;
    int buffer_size;
    char* p_chunk;
    int chunk_fragments;
    char* p;
    int header_size;
    int received;
public:
    ChunkStream(int in_chunk_size, int out_chunk_size, int buffer_size = RTMP_CLINET_MSG_MAX_SIZE);
    virtual ~ChunkStream();
public:
    virtual int GetSize();
    virtual const char* GetBuffer();
public:
    virtual char GetMessageType();
    virtual int GetBodySize();
    virtual const char* GetBody();
    virtual bool Completed();
    virtual void Reset();
public:
    virtual int WriteBegin(char cid, int32_t timestamp, char message_type, int32_t message_stream_id);
    virtual int WriteAMF0String(const char* value);
    virtual int WriteAMF0Boolean(bool value);
    virtual int WriteAMF0Number(double value);
    virtual int WriteAMF0Null();
    virtual int WriteAMF0ObjectStart();
    virtual int WriteAMF0ObjectPropertyName(const char* value);
    virtual int WriteAMF0ObjectEnd();
    virtual int WriteInt16(int32_t value);
    virtual int WriteInt32(int32_t value);
    virtual int WriteEnd();
public:
    virtual int Read(StSocket* socket, ssize_t* pnread);
    virtual int ReadFast(StSocket* socket, ssize_t* pnread);
    virtual int ParseAMF0Type(char required_amf0_type);
    virtual int ParseAMF0String(std::string* value);
    virtual int ParseAMF0Number(double* value);
    virtual int ParseInt16(int16_t* value);
    virtual int ParseInt32(int32_t* value);
private:
    virtual int ReadHeader(StSocket* socket, ssize_t* pnread);
};

class Rtmp
{
private:
    int in_chunk_size;
    int out_chunk_size;
private:
    ChunkStream* current_cs;
    ChunkStream** in_chunk_streams;
    ChunkStream* out_chunk_stream;
// protocol info
private:
    int ack_size;
    int64_t acked_size;
    int64_t recv_size;
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
    virtual int WriteAMF0Null();
    virtual int WriteAMF0ObjectStart();
    virtual int WriteAMF0ObjectPropertyName(const char* value);
    virtual int WriteAMF0ObjectEnd();
    virtual int WriteInt16(int32_t value);
    virtual int WriteInt32(int32_t value);
    virtual int WriteEnd();
public:
    virtual int Read(StSocket* socket);
    virtual int ReadFast(StSocket* socket);
    virtual int ParseAMF0Type(char required_amf0_type);
    virtual int ParseAMF0String(std::string* value);
    virtual int ParseAMF0Number(double* value);
private:
    virtual int ReadHeader(StSocket* socket);
    virtual int FilterPacket(StSocket* socket);
};

class StRtmpClient
{
private:
    Rtmp rtmp;
    int stream_id;
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
    virtual int CreateStream();
    virtual int PlayStram(RtmpUrl* url);
    virtual int DumpAV();
private:
    virtual int BuildConnectApp(RtmpUrl* url);
};

#endif