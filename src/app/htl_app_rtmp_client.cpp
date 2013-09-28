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

#define RtmpStreamSet(p, value)\
    if(p - p_chunk == out_chunk_size){ \
        *p++ = 0xC0 | cid; \
        chunk_fragments++; \
        p_chunk = p; \
    } \
    *p++ = value
#define RtmpStreamRequire(size)\
    if(RTMP_CLINET_MSG_MAX_SIZE - (p - buffer) < size){ \
        return ERROR_RTMP_OVERFLOW; \
    }

Rtmp::Rtmp(){
    in_chunk_size = out_chunk_size = RTMP_DEFAULT_CHUNK_SIZE;
    p = p_chunk = buffer;
    ack_size = -1;
    acked_size = recv_size = 0;
}

Rtmp::~Rtmp(){
}

int Rtmp::GetSize(){
    return p - buffer;
}

const char* Rtmp::GetBuffer(){
    return buffer;
}

char Rtmp::GetMessageType(){
    return message_type;
}

int Rtmp::GetBodySize(){
    return message_length;
}

const char* Rtmp::GetBody(){
    return buffer + header_size;
}

int Rtmp::WriteBegin(char cid, int32_t timestamp, char message_type, int32_t message_stream_id){
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
    if(timestamp > 0xFFFFFF){
        p = buffer + 4;
    }
    
    p_chunk = p;
    header_size = p - buffer;
    
    return ret;
}
    
int Rtmp::WriteAMF0String(const char* value){
    int ret = ERROR_SUCCESS;
    
    int16_t size = strlen(value);
    RtmpStreamRequire(3 + size);
    
    RtmpStreamSet(p, RTMP_AMF0_String);
    
    if((ret = WriteAMF0ObjectPropertyName(value)) != ERROR_SUCCESS){
        return ret;
    }
    
    return ret;
}
    
int Rtmp::WriteAMF0Boolean(bool value){
    int ret = ERROR_SUCCESS;
    
    RtmpStreamRequire(1 + 1);
    
    RtmpStreamSet(p, RTMP_AMF0_Boolean);
    RtmpStreamSet(p, value? 1 : 0);
    
    return ret;
}

int Rtmp::WriteAMF0Number(double value){
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

int Rtmp::WriteAMF0ObjectStart(){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(1);
    
    RtmpStreamSet(p, RTMP_AMF0_Object);
    
    return ret;
}

int Rtmp::WriteAMF0ObjectPropertyName(const char* value){
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

int Rtmp::WriteAMF0ObjectEnd(){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(3);
    
    RtmpStreamSet(p, 0x00);
    RtmpStreamSet(p, 0x00);
    RtmpStreamSet(p, 0x09);
    
    return ret;
}

int Rtmp::WriteInt32(int32_t value){
    int ret = ERROR_SUCCESS;

    RtmpStreamRequire(4);
    
    char* pp = (char*)&value;
    RtmpStreamSet(p, pp[3]);
    RtmpStreamSet(p, pp[2]);
    RtmpStreamSet(p, pp[1]);
    RtmpStreamSet(p, pp[0]);
    
    return ret;
}

int Rtmp::WriteEnd(){
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
    if(timestamp > 0xFFFFFF){
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
    if(timestamp > 0xFFFFFF){
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

int Rtmp::Read(StSocket* socket){
    int ret = ERROR_SUCCESS;
    
    ssize_t nread;
    p = buffer;
    
    // we assume the packet is not interlaced.
    // so the header must be a new stream(fmt===0).
    
    // max header size:
    // 1 bytes basic header
    // 11 bytes message header
    // 4 bytes extended timestamp.
    if((ret = socket->ReadFully(buffer, 12, &nread)) != ERROR_SUCCESS){
        return ret;
    }
    recv_size += nread;
    
    // parse the header in buffer, change the position of p.
    if((ret = ParseHeader(socket)) != ERROR_SUCCESS){
        return ret;
    }
    
    // curent p is the body start position.
    char* pbody = p;
    header_size = p - buffer;
    
    // move p to the end of header.
    p = buffer + nread;
    
    // read all data.
    while(p - pbody < message_length){
        int size_left = message_length - (p - pbody);
        
        if(size_left > in_chunk_size){
            // read the next header.
            size_left = in_chunk_size + 1;
        }
        
        // you can use ReadDrop to drop the message body if not care.
        if(p + size_left > buffer + RTMP_CLINET_MSG_MAX_SIZE){
            ret = ERROR_RTMP_MSG_TOO_BIG;
            Error("message size=%d is too big, available buffer is %d, ret=%d", 
                message_length, (int)(RTMP_CLINET_MSG_MAX_SIZE - (pbody - buffer)), ret);
            return ret;
        }
        
        if((ret = socket->ReadFully(p, size_left, &nread)) != ERROR_SUCCESS){
            return ret;
        }
        recv_size += nread;
        
        // completed.
        if(p + size_left >= pbody + message_length){
            break;
        }
        
        // back to the basic header where fmt===3.
        p += size_left - 1;
        
        // ensure the header fmt===3.
        if((ret = ParseHeader(socket)) != ERROR_SUCCESS){
            return ret;
        }
        
        // ignore the parsed header.
        p--;
    }

    // filter received packet.
    if((ret = FilterPacket(socket)) != ERROR_SUCCESS){
        return ret;
    }
    
    // set p to the body.
    p = buffer + header_size;
    
    return ret;
}

int Rtmp::ParseHeader(StSocket* socket){
    int ret = ERROR_SUCCESS;
    
    // basic header, 1/2/3 bytes.
    char fmt = *p>>6;
    char cid = *p++ & 0x3f;
    if(cid == 0){
        cid = 64;
        cid += *p++;
    }
    else if(cid == 1){
        cid = 64;
        cid += 256 * (*p++);
    }
    Info("fmt=%d, cid=%d", fmt, cid);
    
    // simplify the rtmp, disable the interlaced chunk
    if(fmt == 0){
        this->cid = cid;
    }
    else{
        assert(cid == this->cid);
    }
    
    // message header.
    // see also: ngx_rtmp_recv
    if(fmt <= 2){
        char* pp = (char*)&timestamp;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
        pp[3] = 0;
        
        extended_timestamp = (timestamp == 0xFFFFFF);
        
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
        recv_size += nread;
        
        char* pp = (char*)&timestamp;
        pp[3] = *p++;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
    }
    
    return ret;
}

int Rtmp::FilterPacket(StSocket* socket){
    int ret = ERROR_SUCCESS;

    // set p to the body.
    p = buffer + header_size;
    
    if(message_type == RTMP_MSG_WindowAcknowledgementSize){
        assert(message_length == 4);
        
        char* pp = (char*)&ack_size;
        pp[3] = *p++;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
        Info("set ack size to %d", ack);
    }
    if(message_type == RTMP_MSG_SetChunkSize){
        assert(message_length == 4);
        
        char* pp = (char*)&in_chunk_size;
        pp[3] = *p++;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
        Info("set in_chunk_size to %d", in_chunk_size);
    }
    
    // no ack size, ignore
    if(ack_size <= 0){
        return ret;
    }
    
    // send out ack size immediately
    if(recv_size - acked_size >= ack_size){
        Rtmp rtmp;
        
        if((ret = rtmp.WriteBegin(RTMP_CID_ProtocolControl, 0, RTMP_MSG_Acknowledgement, 0)) != ERROR_SUCCESS){
            return ret;
        }
        if((ret = rtmp.WriteInt32(recv_size)) != ERROR_SUCCESS){
            return ret;
        }
        if((ret = rtmp.WriteEnd()) != ERROR_SUCCESS){
            return ret;
        }
        
        ssize_t nsize;
        if((ret = socket->Write(rtmp.GetBuffer(), rtmp.GetSize(), &nsize)) != ERROR_SUCCESS){
            return ret;
        }
    }

    return ret;
}

StRtmpClient::StRtmpClient(){
    socket = new StSocket();
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
        
        if(rtmp.GetMessageType() == RTMP_MSG_AMF0CommandMessage){
        }
    }
    
    return ret;
}

int StRtmpClient::PlayStram(RtmpUrl* url){
    int ret = ERROR_SUCCESS;
    return ret;
}

int StRtmpClient::DumpAV(){
    int ret = ERROR_SUCCESS;
    return ret;
}

