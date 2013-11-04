#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <assert.h>

#include <string>
#include <sstream>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>

#include <htl_app_rtmp_client.hpp>

/**
5. Protocol Control Messages
RTMP reserves message type IDs 1-7 for protocol control messages.
These messages contain information needed by the RTM Chunk Stream
protocol or RTMP itself. Protocol messages with IDs 1 & 2 are
reserved for usage with RTM Chunk Stream protocol. Protocol messages
with IDs 3-6 are reserved for usage of RTMP. Protocol message with ID
7 is used between edge server and origin server.
*/
#define RTMP_MSG_SetChunkSize 0x01
#define RTMP_MSG_AbortMessage 0x02
#define RTMP_MSG_Acknowledgement 0x03
#define RTMP_MSG_UserControlMessage 0x04
#define RTMP_MSG_WindowAcknowledgementSize 0x05
#define RTMP_MSG_SetPeerBandwidth 0x06
#define RTMP_MSG_EdgeAndOriginServerCommand 0x07
/**
* The server sends this event to test whether the client is reachable. 
* 
* Event data is a 4-byte timestamp, representing the local server time when the server dispatched the command. 
* The client responds with PingResponse on receiving PingRequest.
*/
#define RTMP_MSG_PCUC_PingRequest 0x06

/**
* The client sends this event to the server in response to the ping request. 
* 
* The event data is a 4-byte timestamp, which was received with the PingRequest request.
*/
#define RTMP_MSG_PCUC_PingResponse 0x07
/**
3. Types of messages
The server and the client send messages over the network to
communicate with each other. The messages can be of any type which
includes audio messages, video messages, command messages, shared
object messages, data messages, and user control messages.
3.1. Command message
Command messages carry the AMF-encoded commands between the client
and the server. These messages have been assigned message type value
of 20 for AMF0 encoding and message type value of 17 for AMF3
encoding. These messages are sent to perform some operations like
connect, createStream, publish, play, pause on the peer. Command
messages like onstatus, result etc. are used to inform the sender
about the status of the requested commands. A command message
consists of command name, transaction ID, and command object that
contains related parameters. A client or a server can request Remote
Procedure Calls (RPC) over streams that are communicated using the
command messages to the peer.
*/
#define RTMP_MSG_AMF3CommandMessage 17 // 0x11
#define RTMP_MSG_AMF0CommandMessage 20 // 0x14
/**
3.2. Data message
The client or the server sends this message to send Metadata or any
user data to the peer. Metadata includes details about the
data(audio, video etc.) like creation time, duration, theme and so
on. These messages have been assigned message type value of 18 for
AMF0 and message type value of 15 for AMF3.        
*/
#define RTMP_MSG_AMF0DataMessage 18 // 0x12
#define RTMP_MSG_AMF3DataMessage 15 // 0x0F
/**
3.3. Shared object message
A shared object is a Flash object (a collection of name value pairs)
that are in synchronization across multiple clients, instances, and
so on. The message types kMsgContainer=19 for AMF0 and
kMsgContainerEx=16 for AMF3 are reserved for shared object events.
Each message can contain multiple events.
*/
#define RTMP_MSG_AMF3SharedObject 16 // 0x10
#define RTMP_MSG_AMF0SharedObject 19 // 0x13
/**
3.4. Audio message
The client or the server sends this message to send audio data to the
peer. The message type value of 8 is reserved for audio messages.
*/
#define RTMP_MSG_AudioMessage 8 // 0x08
/* *
3.5. Video message
The client or the server sends this message to send video data to the
peer. The message type value of 9 is reserved for video messages.
These messages are large and can delay the sending of other type of
messages. To avoid such a situation, the video message is assigned
the lowest priority.
*/
#define RTMP_MSG_VideoMessage 9 // 0x09
/**
3.6. Aggregate message
An aggregate message is a single message that contains a list of submessages.
The message type value of 22 is reserved for aggregate
messages.
*/
#define RTMP_MSG_AggregateMessage 22 // 0x16
/**
* the chunk stream id used for some under-layer message,
* for example, the PC(protocol control) message.
*/
#define RTMP_CID_ProtocolControl 0x02

/**
* the AMF0/AMF3 command message, invoke method and return the result, over NetConnection.
* generally use 0x03.
*/
#define RTMP_CID_OverConnection 0x03
/**
* the AMF0/AMF3 command message, invoke method and return the result, over NetConnection, 
* the midst state(we guess).
* rarely used, e.g. onStatus(NetStream.Play.Reset).
*/
#define RTMP_CID_OverConnection2 0x04
/**
* the stream message(amf0/amf3), over NetStream.
* generally use 0x05.
*/
#define RTMP_CID_OverStream 0x05
/**
* the stream message(amf0/amf3), over NetStream, the midst state(we guess).
* rarely used, e.g. play("mp4:mystram.f4v")
*/
#define RTMP_CID_OverStream2 0x08
/**
* the stream message(video), over NetStream
* generally use 0x06.
*/
#define RTMP_CID_Video 0x06
/**
* the stream message(audio), over NetStream.
* generally use 0x07.
*/
#define RTMP_CID_Audio 0x07
// AMF0 marker
#define RTMP_AMF0_Number 0x00
#define RTMP_AMF0_Boolean 0x01
#define RTMP_AMF0_String 0x02
#define RTMP_AMF0_Object 0x03
#define RTMP_AMF0_MovieClip 0x04 // reserved, not supported
#define RTMP_AMF0_Null 0x05
#define RTMP_AMF0_Undefined 0x06
#define RTMP_AMF0_Reference 0x07
#define RTMP_AMF0_EcmaArray 0x08
#define RTMP_AMF0_ObjectEnd 0x09
#define RTMP_AMF0_StrictArray 0x0A
#define RTMP_AMF0_Date 0x0B
#define RTMP_AMF0_LongString 0x0C
#define RTMP_AMF0_UnSupported 0x0D
#define RTMP_AMF0_RecordSet 0x0E // reserved, not supported
#define RTMP_AMF0_XmlDocument 0x0F
#define RTMP_AMF0_TypedObject 0x10
// AVM+ object is the AMF3 object.
#define RTMP_AMF0_AVMplusObject 0x11
// origin array whos data takes the same form as LengthValueBytes
#define RTMP_AMF0_OriginStrictArray 0x20

#define RTMP_DEFAULT_CHUNK_SIZE 128
#define RTMP_EXTENDED_TIMESTAMP 0xFFFFFF

#define RtmpStreamSet(p, value)\
    if(p - p_chunk == out_chunk_size){ \
        *p++ = 0xC0 | cid; \
        chunk_fragments++; \
        p_chunk = p; \
    } \
    *p++ = value
    
#define RtmpStreamRequire(size)\
    if(buffer_size - (p - buffer) < size){ \
        return ERROR_RTMP_OVERFLOW; \
    }
    
ChunkStream::ChunkStream(int in_chunk_size, int out_chunk_size, int buffer_size){
    fmt = cid = 0;
    extended_timestamp = false;
    timestamp = message_length = message_type = message_stream_id = 0;
    
    header_size = received = 0;

    this->in_chunk_size = in_chunk_size;
    this->out_chunk_size = out_chunk_size;
    
    assert(buffer_size > 0);
    buffer = new char[buffer_size];
    this->buffer_size = buffer_size;
    p = p_chunk = buffer;
}

ChunkStream::~ChunkStream(){
    delete[] buffer;
    buffer = NULL;
}

int ChunkStream::GetSize(){
    return p - buffer;
}

const char* ChunkStream::GetBuffer(){
    return buffer;
}

char ChunkStream::GetMessageType(){
    return message_type;
}

int ChunkStream::GetBodySize(){
    return message_length;
}

const char* ChunkStream::GetBody(){
    return buffer + header_size;
}

bool ChunkStream::Completed(){
    return received >= message_length;
}

void ChunkStream::Reset(){
    p = buffer + header_size;
    received = 0;
}

int ChunkStream::WriteBegin(char cid, int32_t timestamp, char message_type, int32_t message_stream_id){
    int ret = ERROR_SUCCESS;
    
    // save data, use them when WriteEnd.
    this->cid = cid;
    this->timestamp = timestamp;
    this->message_type = message_type;
    this->message_stream_id = message_stream_id;

    chunk_fragments = 0;
    
    // begin stream, 12bytes header reserved.
    p = buffer + 12;
    
    // 4bytes extended timestamp
    if(timestamp >= RTMP_EXTENDED_TIMESTAMP){
        p = buffer + 4;
    }
    
    p_chunk = p;
    header_size = p - buffer;
    
    return ret;
}

int ChunkStream::WriteAMF0String(const char* value){
    int ret = ERROR_SUCCESS;
    
    int16_t size = strlen(value);
    RtmpStreamRequire(3 + size);
    
    RtmpStreamSet(p, RTMP_AMF0_String);
    
    if((ret = WriteAMF0ObjectPropertyName(value)) != ERROR_SUCCESS){
        return ret;
    }
    
    return ret;
}
    
int ChunkStream::WriteAMF0Boolean(bool value){
    int ret = ERROR_SUCCESS;
    
    RtmpStreamRequire(1 + 1);
    
    RtmpStreamSet(p, RTMP_AMF0_Boolean);
    RtmpStreamSet(p, value? 1 : 0);
    
    return ret;
}

int ChunkStream::WriteAMF0Number(double value){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(1 + 8);
    
    RtmpStreamSet(p, RTMP_AMF0_Number);
    
    int64_t data;
    memcpy(&data, &value, 8);
    
    char* pp = (char*)&data;
    RtmpStreamSet(p, pp[7]);
    RtmpStreamSet(p, pp[6]);
    RtmpStreamSet(p, pp[5]);
    RtmpStreamSet(p, pp[4]);
    RtmpStreamSet(p, pp[3]);
    RtmpStreamSet(p, pp[2]);
    RtmpStreamSet(p, pp[1]);
    RtmpStreamSet(p, pp[0]);
    
    return ret;
}

int ChunkStream::WriteAMF0Null(){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(1);
    
    RtmpStreamSet(p, RTMP_AMF0_Null);
    
    return ret;
}

int ChunkStream::WriteAMF0ObjectStart(){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(1);
    
    RtmpStreamSet(p, RTMP_AMF0_Object);
    
    return ret;
}

int ChunkStream::WriteAMF0ObjectPropertyName(const char* value){
    int ret = ERROR_SUCCESS;
    
    int16_t size = strlen(value);

    RtmpStreamRequire(2 + size);

    char* pp = (char*)&size;
    RtmpStreamSet(p, pp[1]);
    RtmpStreamSet(p, pp[0]);

    for(int i = 0; i < size; i++){
        RtmpStreamSet(p, value[i]);
    }
    
    return ret;
}

int ChunkStream::WriteAMF0ObjectEnd(){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(3);
    
    RtmpStreamSet(p, 0x00);
    RtmpStreamSet(p, 0x00);
    RtmpStreamSet(p, 0x09);
    
    return ret;
}

int ChunkStream::WriteInt16(int32_t value){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(2);
    
    char* pp = (char*)&value;
    RtmpStreamSet(p, pp[1]);
    RtmpStreamSet(p, pp[0]);
    
    return ret;
}

int ChunkStream::WriteInt32(int32_t value){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(4);
    
    char* pp = (char*)&value;
    RtmpStreamSet(p, pp[3]);
    RtmpStreamSet(p, pp[2]);
    RtmpStreamSet(p, pp[1]);
    RtmpStreamSet(p, pp[0]);
    
    return ret;
}

int ChunkStream::WriteEnd(){
    int ret = ERROR_SUCCESS;
    
    if(RTMP_CLINET_MSG_MAX_SIZE < 16){
        return ERROR_RTMP_OVERFLOW;
    }
    
    // the message length is the offset of p.
    int total_size = p - buffer;
    message_length = total_size - header_size - chunk_fragments;
    
    // reset to header.
    p = buffer;
    
    char* pp = NULL;

    // chunk basic header, 1 bytes
    *p++ = 0x00 | cid; // fmt && cid, big-endian

    // chunk message header, 11 bytes
    // timestamp, 3bytes, big-endian
    if(timestamp >= RTMP_EXTENDED_TIMESTAMP){
        p[2] = 0xFF;
        p[1] = 0xFF;
        p[0] = 0xFF;
    }
    else{
        pp = (char*)&timestamp; 
        *p++ = pp[2];
        *p++ = pp[1];
        *p++ = pp[0];
    }
    // message_length, 3bytes, big-endian
    pp = (char*)&message_length;
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];
    // message_type, 1bytes
    *p++ = message_type;
    // message_length, 3bytes, little-endian
    pp = (char*)&message_stream_id;
    *p++ = pp[0];
    *p++ = pp[1];
    *p++ = pp[2];
    *p++ = pp[3];
    
    // chunk extended timestamp header, 0 or 4 bytes, big-endian
    if(timestamp >= RTMP_EXTENDED_TIMESTAMP){
        pp = (char*)&timestamp; 
        *p++ = pp[3];
        *p++ = pp[2];
        *p++ = pp[1];
        *p++ = pp[0];
    }

    // reset the p to the end of buffer
    p = buffer + total_size;
    
    return ret;
}

int ChunkStream::Read(StSocket* socket, ssize_t* pnread){
    int ret = ERROR_SUCCESS;
    
    if((ret = ReadHeader(socket, pnread)) != ERROR_SUCCESS){
        return ret;
    }
    
    int size_left = message_length - received;
    
    if(size_left > in_chunk_size){
        size_left = in_chunk_size;
    }
    
    // you can use ReadDrop to drop the message body if not care.
    if(p + size_left > buffer + RTMP_CLINET_MSG_MAX_SIZE){
        ret = ERROR_RTMP_MSG_TOO_BIG;
        Error("message size=%d is too big, available buffer is %d, ret=%d", 
            message_length, (int)(RTMP_CLINET_MSG_MAX_SIZE - header_size), ret);
        return ret;
    }

    ssize_t nread;
    if((ret = socket->ReadFully(p, size_left, &nread)) != ERROR_SUCCESS){
        return ret;
    }
    *pnread += nread;
    received += nread;
    
    p += nread;
    
    Info("[read] message header fmt=%d cid=%d type=%d size=%d recv=%d time=%d", fmt, cid, message_type, message_length, received, timestamp);
    
    return ret;
}

int ChunkStream::ReadFast(StSocket* socket, ssize_t* pnread){
    int ret = ERROR_SUCCESS;
    
    if((ret = ReadHeader(socket, pnread)) != ERROR_SUCCESS){
        return ret;
    }
    
    int size_left = message_length - received;
    
    if(size_left > in_chunk_size){
        size_left = in_chunk_size;
    }

    ssize_t nread;
    if((ret = socket->ReadFully(p, size_left, &nread)) != ERROR_SUCCESS){
        return ret;
    }
    *pnread += nread;
    received += nread;
    
    Info("[fast] message header fmt=%d cid=%d type=%d size=%d recv=%d time=%d", fmt, cid, message_type, message_length, received, timestamp);
    
    return ret;
}

int ChunkStream::ParseAMF0Type(char required_amf0_type){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(1);
    
    char amf0_type = *p++;
        
    if(amf0_type != required_amf0_type){
        ret = ERROR_RTMP_INVALID_RESPONSE;
        Error("invalid amf0 type=%d, required=%d, ret=%d", amf0_type, required_amf0_type, ret);
        return ret;
    }
    
    return ret;
}

int ChunkStream::ParseAMF0String(std::string* value){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(2);
    
    int16_t size;
    char* pp = (char*)&size;
    pp[1] = *p++;
    pp[0] = *p++;
    
    RtmpStreamRequire(size);
    
    value->append(p, size);
    p += size;
    
    return ret;
}

int ChunkStream::ParseAMF0Number(double* value){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(8);
    
    int64_t data;
    char* pp = (char*)&data;
    pp[7] = *p++;
    pp[6] = *p++;
    pp[5] = *p++;
    pp[4] = *p++;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;
    
    memcpy(value, &data, 8);
    
    return ret;
}

int ChunkStream::ParseInt16(int16_t* value){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(2);
    
    char* pp = (char*)value;
    pp[1] = *p++;
    pp[0] = *p++;
    
    return ret;
}

int ChunkStream::ParseInt32(int32_t* value){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(4);
    
    char* pp = (char*)value;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;
    
    return ret;
}

int ChunkStream::ReadHeader(StSocket* socket, ssize_t* pnread){
    int ret = ERROR_SUCCESS;
    
    static char mh_sizes[] = {11, 7, 1, 0};
    char mh_size = mh_sizes[(int)fmt];
    
    if(fmt == 0){
        p = buffer;
        received = 0;
    }
    
    if(mh_size > 0){
        ssize_t nread;
        if((ret = socket->ReadFully(p, mh_size, &nread)) != ERROR_SUCCESS){
            return ret;
        }
        *pnread += nread;
    }
    
    // message header.
    // see also: ngx_rtmp_recv
    if(fmt <= 2){
        char* pp = (char*)&timestamp;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
        pp[3] = 0;
        
        extended_timestamp = (timestamp == RTMP_EXTENDED_TIMESTAMP);
        
        if(fmt <= 1){
            pp = (char*)&message_length;
            pp[2] = *p++;
            pp[1] = *p++;
            pp[0] = *p++;
            pp[3] = 0;
            
            message_type = *p++;
            
            if(fmt == 0){
                pp = (char*)&message_stream_id;
                pp[0] = *p++;
                pp[1] = *p++;
                pp[2] = *p++;
                pp[3] = *p++;
            }
        }
    }

    if(extended_timestamp){
        ssize_t nread;
        if((ret = socket->ReadFully(p, 4, &nread)) != ERROR_SUCCESS){
            return ret;
        }
        *pnread += nread;
        
        char* pp = (char*)&timestamp;
        pp[3] = *p++;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
    }
    
    if(fmt == 0){
        header_size = p - buffer;
    }
    
    return ret;
}

Rtmp::Rtmp(){
    ack_size = -1;
    acked_size = recv_size = 0;
    
    in_chunk_size = out_chunk_size = RTMP_DEFAULT_CHUNK_SIZE;
    out_chunk_stream = new ChunkStream(in_chunk_size, out_chunk_size);

    in_chunk_streams = new ChunkStream*[RTMP_MAX_CHUNK_STREAMS];
    for(int i = 0; i < RTMP_MAX_CHUNK_STREAMS; i++){
        in_chunk_streams[i] = NULL;
    }
    current_cs = NULL;
}

Rtmp::~Rtmp(){
    delete out_chunk_stream;
    out_chunk_stream = NULL;

    delete[] in_chunk_streams;
    in_chunk_streams = NULL;
}

int Rtmp::GetSize(){
    return current_cs->GetSize();
}

const char* Rtmp::GetBuffer(){
    return current_cs->GetBuffer();
}

char Rtmp::GetMessageType(){
    return current_cs->message_type;
}

int Rtmp::GetBodySize(){
    return current_cs->message_length;
}

const char* Rtmp::GetBody(){
    return current_cs->GetBody();
}

int Rtmp::WriteBegin(char cid, int32_t timestamp, char message_type, int32_t message_stream_id){
    current_cs = out_chunk_stream;
    
    return current_cs->WriteBegin(cid, timestamp, message_type, message_stream_id);
}

int Rtmp::WriteAMF0String(const char* value){
    return current_cs->WriteAMF0String(value);
}
    
int Rtmp::WriteAMF0Boolean(bool value){
    return current_cs->WriteAMF0Boolean(value);
}

int Rtmp::WriteAMF0Number(double value){
    return current_cs->WriteAMF0Number(value);
}

int Rtmp::WriteAMF0Null(){
    return current_cs->WriteAMF0Null();
}

int Rtmp::WriteAMF0ObjectStart(){
    return current_cs->WriteAMF0ObjectStart();
}

int Rtmp::WriteAMF0ObjectPropertyName(const char* value){
    return current_cs->WriteAMF0ObjectPropertyName(value);
}

int Rtmp::WriteAMF0ObjectEnd(){
    return current_cs->WriteAMF0ObjectEnd();
}

int Rtmp::WriteInt16(int32_t value){
    return current_cs->WriteInt16(value);
}

int Rtmp::WriteInt32(int32_t value){
    return current_cs->WriteInt32(value);
}

int Rtmp::WriteEnd(){
    return current_cs->WriteEnd();
}

int Rtmp::Read(StSocket* socket){
    int ret = ERROR_SUCCESS;
    
    while(true){
        if((ret = ReadHeader(socket)) != ERROR_SUCCESS){
            return ret;
        }
        
        ssize_t nread = 0;
        if((ret = current_cs->Read(socket, &nread)) != ERROR_SUCCESS){
            return ret;
        }
        recv_size += nread;
        
        if(!current_cs->Completed()){
            continue;
        }
        
        current_cs->Reset();
        if((ret = FilterPacket(socket)) != ERROR_SUCCESS){
            return ret;
        }
        
        current_cs->Reset();
        break;
    }
    
    return ret;
}

int Rtmp::ReadFast(StSocket* socket){
    int ret = ERROR_SUCCESS;
    
    while(true){
        if((ret = ReadHeader(socket)) != ERROR_SUCCESS){
            return ret;
        }
        
        ssize_t nread = 0;
        if((ret = current_cs->ReadFast(socket, &nread)) != ERROR_SUCCESS){
            return ret;
        }
        recv_size += nread;
        
        if(!current_cs->Completed()){
            continue;
        }
        
        current_cs->Reset();
        if((ret = FilterPacket(socket)) != ERROR_SUCCESS){
            return ret;
        }
        
        current_cs->Reset();
        break;
    }
        
    return ret;
}

int Rtmp::ParseAMF0Type(char required_amf0_type){
    return current_cs->ParseAMF0Type(required_amf0_type);
}

int Rtmp::ParseAMF0String(std::string* value){
    return current_cs->ParseAMF0String(value);
}

int Rtmp::ParseAMF0Number(double* value){
    return current_cs->ParseAMF0Number(value);
}

int Rtmp::ReadHeader(StSocket* socket){
    int ret = ERROR_SUCCESS;
    
    ssize_t nread;
    
    // read the basic header.
    char bh;
    if((ret = socket->Read(&bh, 1, &nread)) != ERROR_SUCCESS){
        return ret;
    }
    recv_size += nread;
    
    char fmt = (bh >> 6) & 0x03;
    int16_t cid = bh & 0x3f;
    
    if(cid == 0){
        if((ret = socket->Read(&bh, 1, &nread)) != ERROR_SUCCESS){
            return ret;
        }
        recv_size += nread;

        cid = 64;
        cid += bh;
    }
    else if(cid == 1){
        if((ret = socket->Read(&bh, 1, &nread)) != ERROR_SUCCESS){
            return ret;
        }
        recv_size += nread;

        cid = 64;
        cid += 256 * bh;
    }
    
    assert(cid < RTMP_MAX_CHUNK_STREAMS);
    current_cs = in_chunk_streams[cid];
    if(current_cs == NULL){
        current_cs = in_chunk_streams[cid] = new ChunkStream(in_chunk_size, out_chunk_size, RTMP_CLINET_MSG_MAX_SIZE);
    }

    Info("fmt=%d, cid=%d, cache=(fmt=%d, cid=%d, size=%d, type=%d)", fmt, cid,
        current_cs->fmt, current_cs->cid, current_cs->message_length, current_cs->message_type);
    
    current_cs->fmt = fmt;
    current_cs->cid = cid;
    
    return ret;
}

int Rtmp::FilterPacket(StSocket* socket){
    int ret = ERROR_SUCCESS;
    
    if(current_cs->message_type == RTMP_MSG_WindowAcknowledgementSize){
        assert(current_cs->message_length == 4);
        
        if((ret = current_cs->ParseInt32(&ack_size)) != ERROR_SUCCESS){
            return ret;
        }
        Info("set ack size to %d", ack_size);
    }
    if(current_cs->message_type == RTMP_MSG_SetChunkSize){
        assert(current_cs->message_length == 4);
        
        if((ret = current_cs->ParseInt32(&in_chunk_size)) != ERROR_SUCCESS){
            return ret;
        }
        Info("set in_chunk_size to %d", in_chunk_size);
        
        for(int i = 0; i < RTMP_MAX_CHUNK_STREAMS; i++){
            if(in_chunk_streams[i] == NULL){
                continue;
            }
            
            in_chunk_streams[i]->in_chunk_size = in_chunk_size;
        }
        out_chunk_stream->in_chunk_size = in_chunk_size;
    }
    
    // send out ack size immediately
    if(ack_size > 0 && recv_size - acked_size >= ack_size){
        if((ret = out_chunk_stream->WriteBegin(RTMP_CID_ProtocolControl, 0, RTMP_MSG_Acknowledgement, 0)) != ERROR_SUCCESS){
            return ret;
        }
        if((ret = out_chunk_stream->WriteInt32(recv_size)) != ERROR_SUCCESS){
            return ret;
        }
        if((ret = out_chunk_stream->WriteEnd()) != ERROR_SUCCESS){
            return ret;
        }
        
        ssize_t nsize;
        if((ret = socket->Write(out_chunk_stream->GetBuffer(), out_chunk_stream->GetSize(), &nsize)) != ERROR_SUCCESS){
            return ret;
        }
    }
    
    // response ping.
    if(current_cs->message_type == RTMP_MSG_UserControlMessage){
        int16_t eventType;
        if((ret = current_cs->ParseInt16(&eventType)) != ERROR_SUCCESS){
            return ret;
        }
        
        if(eventType == RTMP_MSG_PCUC_PingRequest){
            int32_t timestamp;
            if((ret = current_cs->ParseInt32(&timestamp)) != ERROR_SUCCESS){
                return ret;
            }
            
            if((ret = out_chunk_stream->WriteBegin(RTMP_CID_ProtocolControl, 0, RTMP_MSG_UserControlMessage, 0)) != ERROR_SUCCESS){
                return ret;
            }
            if((ret = out_chunk_stream->WriteInt16(RTMP_MSG_PCUC_PingResponse)) != ERROR_SUCCESS){
                return ret;
            }
            if((ret = out_chunk_stream->WriteInt32(timestamp)) != ERROR_SUCCESS){
                return ret;
            }
            if((ret = out_chunk_stream->WriteEnd()) != ERROR_SUCCESS){
                return ret;
            }
            
            ssize_t nsize;
            if((ret = socket->Write(out_chunk_stream->GetBuffer(), out_chunk_stream->GetSize(), &nsize)) != ERROR_SUCCESS){
                return ret;
            }
        }
    }

    return ret;
}

StRtmpClient::StRtmpClient(){
    socket = new StSocket();
    stream_id = 0;
}

StRtmpClient::~StRtmpClient(){
    delete socket;
    socket = NULL;
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
    
    if((ret = ConnectApp(url)) != ERROR_SUCCESS){
        Error("rtmp client connect tcUrl failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client connect tcUrl(%s) success", url->GetTcUrl());
    
    if((ret = CreateStream()) != ERROR_SUCCESS){
        Error("rtmp client create stream failed. ret=%d", ret);
        return ret;
    }
    Info("rtmp client create stream(%d) success", stream_id);
    
    if((ret = PlayStram(url)) != ERROR_SUCCESS){
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
    
    if(socket->Status() == SocketConnected){
        return ret;
    }

    string ip;
    if((ret = StUtility::DnsResolve(url->GetHost(), ip)) != ERROR_SUCCESS){
        Error("dns resolve failed. ret=%d", ret);
        return ret;
    }
    
    if((ret = socket->Connect(ip.c_str(), url->GetPort())) != ERROR_SUCCESS){
        Error("connect to server failed. ret=%d", ret);
        return ret;
    }
    
    Info("socket connected on url %s", url->GetUrl());
    
    return ret;
}

int StRtmpClient::Handshake(){
    int ret = ERROR_SUCCESS;
    
    ssize_t nsize;
    
    char buf[1537];
    buf[0] = 0x03; // plain text.

    char* c0c1 = buf;
    if((ret = socket->Write(c0c1, 1537, &nsize)) != ERROR_SUCCESS){
        return ret;
    }

    char* s0s1 = buf;
    if((ret = socket->ReadFully(s0s1, 1537, &nsize)) != ERROR_SUCCESS){
        return ret;
    }
    char* s2 = buf;
    if((ret = socket->ReadFully(s2, 1536, &nsize)) != ERROR_SUCCESS){
        return ret;
    }
    
    char* c2 = buf;
    if((ret = socket->Write(c2, 1536, &nsize)) != ERROR_SUCCESS){
        return ret;
    }
    
    return ret;
}

int StRtmpClient::ConnectApp(RtmpUrl* url){
    int ret = ERROR_SUCCESS;

    if((ret = BuildConnectApp(url)) != ERROR_SUCCESS){
        return ret;
    }
    
    ssize_t nsize;
    
    // send connect(tcUrl) packet.
    if((ret = socket->Write(rtmp.GetBuffer(), rtmp.GetSize(), &nsize)) != ERROR_SUCCESS){
        return ret;
    }
    // recv response
    while(true){
        if((ret = rtmp.Read(socket)) != ERROR_SUCCESS){
            return ret;
        }
        
        if(rtmp.GetMessageType() != RTMP_MSG_AMF0CommandMessage){
            Info("ignore message type=%d", rtmp.GetMessageType());
            continue;
        }
        
        if((ret = rtmp.ParseAMF0Type(RTMP_AMF0_String)) != ERROR_SUCCESS){
            return ret;
        }

        string cmd;
        if((ret = rtmp.ParseAMF0String(&cmd)) != ERROR_SUCCESS){
            return ret;
        }
        
        if(cmd != "_result"){
            Info("ignore amf0 cmd=%s", cmd.c_str());
            continue;
        }
        
        Info("connect app completed");
        break;
    }
    
    return ret;
}

int StRtmpClient::CreateStream(){
    int ret = ERROR_SUCCESS;

    if((ret = rtmp.WriteBegin(RTMP_CID_OverConnection, 0, RTMP_MSG_AMF0CommandMessage, 0)) != ERROR_SUCCESS){
        return ret;
    }
    
    if((ret = rtmp.WriteAMF0String("createStream")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0Number(2.0)) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0Null()) != ERROR_SUCCESS){
        return ret;
    }
    
    if((ret = rtmp.WriteEnd()) != ERROR_SUCCESS){
        return ret;
    }
    
    ssize_t nsize;
    
    // send createStream packet.
    if((ret = socket->Write(rtmp.GetBuffer(), rtmp.GetSize(), &nsize)) != ERROR_SUCCESS){
        return ret;
    }
    // recv response
    while(true){
        if((ret = rtmp.Read(socket)) != ERROR_SUCCESS){
            return ret;
        }
        
        if(rtmp.GetMessageType() != RTMP_MSG_AMF0CommandMessage){
            Info("ignore message type=%d", rtmp.GetMessageType());
            continue;
        }
        
        // command name
        if((ret = rtmp.ParseAMF0Type(RTMP_AMF0_String)) != ERROR_SUCCESS){
            return ret;
        }
        string cmd;
        if((ret = rtmp.ParseAMF0String(&cmd)) != ERROR_SUCCESS){
            return ret;
        }
        if(cmd != "_result"){
            Info("ignore amf0 cmd=%s", cmd.c_str());
            continue;
        }
        
        // transaction id.
        if((ret = rtmp.ParseAMF0Type(RTMP_AMF0_Number)) != ERROR_SUCCESS){
            return ret;
        }
        double tid;
        if((ret = rtmp.ParseAMF0Number(&tid)) != ERROR_SUCCESS){
            return ret;
        }
        
        // null
        if((ret = rtmp.ParseAMF0Type(RTMP_AMF0_Null)) != ERROR_SUCCESS){
            return ret;
        }
        
        // stream id
        if((ret = rtmp.ParseAMF0Type(RTMP_AMF0_Number)) != ERROR_SUCCESS){
            return ret;
        }
        double sid;
        if((ret = rtmp.ParseAMF0Number(&sid)) != ERROR_SUCCESS){
            return ret;
        }
        
        stream_id = (int)sid;
        
        Info("create stream completed, stream_id=%d", stream_id);
        break;
    }
    
    return ret;
}

int StRtmpClient::PlayStram(RtmpUrl* url){
    int ret = ERROR_SUCCESS;

    if((ret = rtmp.WriteBegin(RTMP_CID_OverConnection, 0, RTMP_MSG_AMF0CommandMessage, stream_id)) != ERROR_SUCCESS){
        return ret;
    }
    
    if((ret = rtmp.WriteAMF0String("play")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0Number(0)) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0Null()) != ERROR_SUCCESS){
        return ret;
    }
    
    // streamName
    if((ret = rtmp.WriteAMF0String(url->GetStream())) != ERROR_SUCCESS){
        return ret;
    }
    // start
    if((ret = rtmp.WriteAMF0Number(-2)) != ERROR_SUCCESS){
        return ret;
    }
    
    if((ret = rtmp.WriteEnd()) != ERROR_SUCCESS){
        return ret;
    }
    
    ssize_t nsize;
    
    // send createStream packet.
    if((ret = socket->Write(rtmp.GetBuffer(), rtmp.GetSize(), &nsize)) != ERROR_SUCCESS){
        return ret;
    }
    
    return ret;
}

int StRtmpClient::DumpAV(){
    int ret = ERROR_SUCCESS;

    // recv response
    while(true){
        if((ret = rtmp.ReadFast(socket)) != ERROR_SUCCESS){
            return ret;
        }
        
        Info("get message type=%d, size=%d", rtmp.GetMessageType(), rtmp.GetBodySize());
    }
    
    return ret;
}

int StRtmpClient::BuildConnectApp(RtmpUrl* url){
    int ret = ERROR_SUCCESS;

    if((ret = rtmp.WriteBegin(RTMP_CID_OverConnection, 0, RTMP_MSG_AMF0CommandMessage, 0)) != ERROR_SUCCESS){
        return ret;
    }
    
    if((ret = rtmp.WriteAMF0String("connect")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0Number(1.0)) != ERROR_SUCCESS){
        return ret;
    }
    
    if((ret = rtmp.WriteAMF0ObjectStart()) != ERROR_SUCCESS){
        return ret;
    }
    // Property 'app' String 'live'
    if((ret = rtmp.WriteAMF0ObjectPropertyName("app")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0String(url->GetApp())) != ERROR_SUCCESS){
        return ret;
    }
    // Property 'flashVer' String 'LNX 11,2,202,235'
    if((ret = rtmp.WriteAMF0ObjectPropertyName("flashVer")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0String("LNX 11,2,202,235")) != ERROR_SUCCESS){
        return ret;
    }
    // Property 'swfUrl' String 'http://192.168.100.145/debugPlayer/RtmpPlayer.swf'
    if((ret = rtmp.WriteAMF0ObjectPropertyName("swfUrl")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0String("http://127.0.0.1/debugPlayer/RtmpPlayer.swf")) != ERROR_SUCCESS){
        return ret;
    }
    // Property 'tcUrl' String 'rtmp://192.168.100.145:1935/live'
    if((ret = rtmp.WriteAMF0ObjectPropertyName("tcUrl")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0String(url->GetTcUrl())) != ERROR_SUCCESS){
        return ret;
    }
    // Property 'fpad' Boolean false
    if((ret = rtmp.WriteAMF0ObjectPropertyName("fpad")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0Boolean(false)) != ERROR_SUCCESS){
        return ret;
    }
    // Property 'capabilities' Number 239
    if((ret = rtmp.WriteAMF0ObjectPropertyName("capabilities")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0Number(239.0)) != ERROR_SUCCESS){
        return ret;
    }
    // Property 'audioCodecs' Number 3575
    if((ret = rtmp.WriteAMF0ObjectPropertyName("audioCodecs")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0Number(3575.0)) != ERROR_SUCCESS){
        return ret;
    }
    // Property 'videoCodecs' Number 252
    if((ret = rtmp.WriteAMF0ObjectPropertyName("videoCodecs")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0Number(252.0)) != ERROR_SUCCESS){
        return ret;
    }
    // Property 'videoFunction' Number 1
    if((ret = rtmp.WriteAMF0ObjectPropertyName("videoFunction")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0Number(1.0)) != ERROR_SUCCESS){
        return ret;
    }
    // Property 'pageUrl' String 'http://192.168.100.145/debugPlayer/player.html'
    if((ret = rtmp.WriteAMF0ObjectPropertyName("pageUrl")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0String("http://127.0.0.1/debugPlayer/player.html")) != ERROR_SUCCESS){
        return ret;
    }
    // Property 'objectEncoding' Number 3
    if((ret = rtmp.WriteAMF0ObjectPropertyName("objectEncoding")) != ERROR_SUCCESS){
        return ret;
    }
    if((ret = rtmp.WriteAMF0Number(0.0)) != ERROR_SUCCESS){
        return ret;
    }
    // EOF object
    if((ret = rtmp.WriteAMF0ObjectEnd()) != ERROR_SUCCESS){
        return ret;
    }

    if((ret = rtmp.WriteEnd()) != ERROR_SUCCESS){
        return ret;
    }
    
    return ret;
}


