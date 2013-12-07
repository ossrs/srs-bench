#ifndef _htl_app_rtmp_protocol_hpp
#define _htl_app_rtmp_protocol_hpp

/*
#include <htl_app_rtmp_protocol.hpp>
*/

#include <st.h>
#include <vector>
#include <map>

/*
The MIT License (MIT)

Copyright (c) 2013 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
class SrsProtocol;
class ISrsMessage;
class SrsCommonMessage;
class SrsCreateStreamPacket;
class SrsFMLEStartPacket;
class SrsPublishPacket;
class SrsSharedPtrMessage;
class SrsOnMetaDataPacket;

#if 1
	#define srs_verbose(msg, ...) (void)0
	#define srs_info(msg, ...) (void)0
	#define srs_trace(msg, ...) (void)0
	#define srs_warn(msg, ...) (void)0
	#define srs_error(msg, ...) (void)0
#endif

// donot use openssl, use simple handshake.
#undef SRS_SSL

#include <assert.h>
#define srs_assert(expression) assert(expression)

// current release version
#define RTMP_SIG_SRS_VERSION "0.5.0"
// server info.
#define RTMP_SIG_SRS_KEY "srs"
#define RTMP_SIG_SRS_ROLE "origin server"
#define RTMP_SIG_SRS_NAME RTMP_SIG_SRS_KEY"(simple rtmp server)"
#define RTMP_SIG_SRS_URL "https://"RTMP_SIG_SRS_URL_SHORT
#define RTMP_SIG_SRS_URL_SHORT "github.com/winlinvip/simple-rtmp-server"
#define RTMP_SIG_SRS_WEB "http://blog.csdn.net/win_lin"
#define RTMP_SIG_SRS_EMAIL "winterserver@126.com"
#define RTMP_SIG_SRS_LICENSE "The MIT License (MIT)"
#define RTMP_SIG_SRS_COPYRIGHT "Copyright (c) 2013 winlin"
#define RTMP_SIG_SRS_CONTRIBUTOR "winlin"

// default vhost for rtmp
#define RTMP_VHOST_DEFAULT "__defaultVhost__"

#define SRS_LOCALHOST "127.0.0.1"
#define RTMP_DEFAULT_PORT 1935
#define RTMP_DEFAULT_PORTS "1935"

#define SRS_CONF_DEFAULT_HLS_PATH "./objs/nginx/html"
#define SRS_CONF_DEFAULT_HLS_FRAGMENT 10
#define SRS_CONF_DEFAULT_HLS_WINDOW 60
// in ms, for HLS aac sync time.
#define SRS_CONF_DEFAULT_AAC_SYNC 100
// in ms, for HLS aac flush the audio
#define SRS_CONF_DEFAULT_AAC_DELAY 300

#define ERROR_SUCCESS 					0

#define ERROR_ST_SET_EPOLL 				100
#ifndef ERROR_ST_INITIALIZE
#define ERROR_ST_INITIALIZE 			101
#endif
#define ERROR_ST_OPEN_SOCKET			102
#define ERROR_ST_CREATE_LISTEN_THREAD	103
#define ERROR_ST_CREATE_CYCLE_THREAD	104
#define ERROR_ST_CREATE_FORWARD_THREAD	105
#define ERROR_ST_CONNECT				106

#define ERROR_SOCKET_CREATE 			200
#define ERROR_SOCKET_SETREUSE 			201
#define ERROR_SOCKET_BIND 				202
#define ERROR_SOCKET_LISTEN 			203
#define ERROR_SOCKET_CLOSED 			204
#define ERROR_SOCKET_GET_PEER_NAME		205
#define ERROR_SOCKET_GET_PEER_IP		206
#define ERROR_SOCKET_READ				207
#define ERROR_SOCKET_READ_FULLY			208
#define ERROR_SOCKET_WRITE				209
#define ERROR_SOCKET_WAIT				210
#define ERROR_SOCKET_TIMEOUT			211

#define ERROR_RTMP_PLAIN_REQUIRED		300
#define ERROR_RTMP_CHUNK_START			301
#define ERROR_RTMP_MSG_INVLIAD_SIZE		302
#define ERROR_RTMP_AMF0_DECODE			303
#define ERROR_RTMP_AMF0_INVALID			304
#define ERROR_RTMP_REQ_CONNECT			305
#define ERROR_RTMP_REQ_TCURL			306
#define ERROR_RTMP_MESSAGE_DECODE		307
#define ERROR_RTMP_MESSAGE_ENCODE		308
#define ERROR_RTMP_AMF0_ENCODE			309
#define ERROR_RTMP_CHUNK_SIZE			310
#define ERROR_RTMP_TRY_SIMPLE_HS		311
#define ERROR_RTMP_CH_SCHEMA			312
#define ERROR_RTMP_PACKET_SIZE			313
#define ERROR_RTMP_VHOST_NOT_FOUND		314
#define ERROR_RTMP_ACCESS_DENIED		315
#define ERROR_RTMP_HANDSHAKE			316
#define ERROR_RTMP_NO_REQUEST			317

#define ERROR_SYSTEM_STREAM_INIT		400
#define ERROR_SYSTEM_PACKET_INVALID		401
#define ERROR_SYSTEM_CLIENT_INVALID		402
#define ERROR_SYSTEM_ASSERT_FAILED		403
#define ERROR_SYSTEM_SIZE_NEGATIVE		404
#define ERROR_SYSTEM_CONFIG_INVALID		405
#define ERROR_SYSTEM_CONFIG_DIRECTIVE	406
#define ERROR_SYSTEM_CONFIG_BLOCK_START	407
#define ERROR_SYSTEM_CONFIG_BLOCK_END	408
#define ERROR_SYSTEM_CONFIG_EOF			409
#define ERROR_SYSTEM_STREAM_BUSY		410
#define ERROR_SYSTEM_IP_INVALID			411
#define ERROR_SYSTEM_CONFIG_TOO_LARGE	412
#define ERROR_SYSTEM_FORWARD_LOOP		413
#define ERROR_SYSTEM_WAITPID			414

// see librtmp.
// failed when open ssl create the dh
#define ERROR_OpenSslCreateDH			500
// failed when open ssl create the Private key.
#define ERROR_OpenSslCreateP			501
// when open ssl create G.
#define ERROR_OpenSslCreateG			502
// when open ssl parse P1024
#define ERROR_OpenSslParseP1024			503
// when open ssl set G
#define ERROR_OpenSslSetG				504
// when open ssl generate DHKeys
#define ERROR_OpenSslGenerateDHKeys		505
// when open ssl share key already computed.
#define ERROR_OpenSslShareKeyComputed	506
// when open ssl get shared key size.
#define ERROR_OpenSslGetSharedKeySize	507
// when open ssl get peer public key.
#define ERROR_OpenSslGetPeerPublicKey	508
// when open ssl compute shared key.
#define ERROR_OpenSslComputeSharedKey	509
// when open ssl is invalid DH state.
#define ERROR_OpenSslInvalidDHState		510
// when open ssl copy key
#define ERROR_OpenSslCopyKey			511
// when open ssl sha256 digest key invalid size.
#define ERROR_OpenSslSha256DigestSize	512

#define ERROR_HLS_METADATA				600
#define ERROR_HLS_DECODE_ERROR			601
#define ERROR_HLS_CREATE_DIR			602
#define ERROR_HLS_OPEN_FAILED			603
#define ERROR_HLS_WRITE_FAILED			604
#define ERROR_HLS_AAC_FRAME_LENGTH		605
#define ERROR_HLS_AVC_SAMPLE_SIZE		606

#define ERROR_ENCODER_VCODEC			700
#define ERROR_ENCODER_OUTPUT			701
#define ERROR_ENCODER_ACHANNELS			702
#define ERROR_ENCODER_ASAMPLE_RATE		703
#define ERROR_ENCODER_ABITRATE			704
#define ERROR_ENCODER_ACODEC			705
#define ERROR_ENCODER_VPRESET			706
#define ERROR_ENCODER_VPROFILE			707
#define ERROR_ENCODER_VTHREADS			708
#define ERROR_ENCODER_VHEIGHT			709
#define ERROR_ENCODER_VWIDTH			710
#define ERROR_ENCODER_VFPS				711
#define ERROR_ENCODER_VBITRATE			712
#define ERROR_ENCODER_FORK				713
#define ERROR_ENCODER_LOOP				714
#define ERROR_ENCODER_OPEN				715
#define ERROR_ENCODER_DUP2				716

// free the p and set to NULL.
// p must be a T*.
#define srs_freep(p) \
	if (p) { \
		delete p; \
		p = NULL; \
	} \
	(void)0
// free the p which represents a array
#define srs_freepa(p) \
	if (p) { \
		delete[] p; \
		p = NULL; \
	} \
	(void)0

// compare
#define srs_min(a, b) (((a) < (b))? (a) : (b))
#define srs_max(a, b) (((a) < (b))? (b) : (a))

class SrsSocket;

/**
* the buffer provices bytes cache for protocol. generally, 
* protocol recv data from socket, put into buffer, decode to RTMP message.
* protocol encode RTMP message to bytes, put into buffer, send to socket.
*/
class SrsBuffer
{
private:
	std::vector<char> data;
public:
	SrsBuffer();
	virtual ~SrsBuffer();
public:
	virtual int size();
	virtual char* bytes();
	virtual void erase(int size);
private:
	virtual void append(char* bytes, int size);
public:
	virtual int ensure_buffer_bytes(SrsSocket* skt, int required_size);
};

class SrsStream
{
private:
	char* p;
	char* pp;
	char* bytes;
	int size;
public:
	SrsStream();
	virtual ~SrsStream();
public:
	/**
	* initialize the stream from bytes.
	* @_bytes, must not be NULL, or return error.
	* @_size, must be positive, or return error.
	* @remark, stream never free the _bytes, user must free it.
	*/
	virtual int initialize(char* _bytes, int _size);
	/**
	* reset the position to beginning.
	*/
	virtual void reset();
	/**
	* whether stream is empty.
	* if empty, never read or write.
	*/
	virtual bool empty();
	/**
	* whether required size is ok.
	* @return true if stream can read/write specified required_size bytes.
	*/
	virtual bool require(int required_size);
	/**
	* to skip some size.
	* @size can be any value. positive to forward; nagetive to backward.
	*/
	virtual void skip(int size);
	/**
	* tell the current pos.
	*/
	virtual int pos();
	/**
	* left size of bytes.
	*/
	virtual int left();
	virtual char* current();
public:
	/**
	* get 1bytes char from stream.
	*/
	virtual int8_t read_1bytes();
	/**
	* get 2bytes int from stream.
	*/
	virtual int16_t read_2bytes();
	/**
	* get 3bytes int from stream.
	*/
	virtual int32_t read_3bytes();
	/**
	* get 4bytes int from stream.
	*/
	virtual int32_t read_4bytes();
	/**
	* get 8bytes int from stream.
	*/
	virtual int64_t read_8bytes();
	/**
	* get string from stream, length specifies by param len.
	*/
	virtual std::string read_string(int len);
public:
	/**
	* write 1bytes char to stream.
	*/
	virtual void write_1bytes(int8_t value);
	/**
	* write 2bytes int to stream.
	*/
	virtual void write_2bytes(int16_t value);
	/**
	* write 4bytes int to stream.
	*/
	virtual void write_4bytes(int32_t value);
	/**
	* write 8bytes int to stream.
	*/
	virtual void write_8bytes(int64_t value);
	/**
	* write string to stream
	*/
	virtual void write_string(std::string value);
};

/**
* the socket provides TCP socket over st,
* that is, the sync socket mechanism.
*/
class SrsSocket
{
private:
	int64_t recv_timeout;
	int64_t send_timeout;
	int64_t recv_bytes;
	int64_t send_bytes;
	int64_t start_time_ms;
    st_netfd_t stfd;
public:
    SrsSocket(st_netfd_t client_stfd);
    virtual ~SrsSocket();
public:
	virtual void set_recv_timeout(int64_t timeout_us);
	virtual int64_t get_recv_timeout();
	virtual void set_send_timeout(int64_t timeout_us);
	virtual int64_t get_recv_bytes();
	virtual int64_t get_send_bytes();
	virtual int get_recv_kbps();
	virtual int get_send_kbps();
public:
    virtual int read(const void* buf, size_t size, ssize_t* nread);
    virtual int read_fully(const void* buf, size_t size, ssize_t* nread);
    virtual int write(const void* buf, size_t size, ssize_t* nwrite);
    virtual int writev(const iovec *iov, int iov_size, ssize_t* nwrite);
};

class SrsSocket;
class SrsComplexHandshake;

/**
* try complex handshake, if failed, fallback to simple handshake.
*/
class SrsSimpleHandshake
{
public:
	SrsSimpleHandshake();
	virtual ~SrsSimpleHandshake();
public:
	/**
	* simple handshake.
	* @param complex_hs, try complex handshake first, 
	* 		if failed, rollback to simple handshake.
	*/
	virtual int handshake_with_client(SrsSocket& skt, SrsComplexHandshake& complex_hs);
	virtual int handshake_with_server(SrsSocket& skt, SrsComplexHandshake& complex_hs);
};

/**
* rtmp complex handshake,
* @see also crtmp(crtmpserver) or librtmp,
* @see also: http://blog.csdn.net/win_lin/article/details/13006803
*/
class SrsComplexHandshake
{
public:
	SrsComplexHandshake();
	virtual ~SrsComplexHandshake();
public:
	/**
	* complex hanshake.
	* @_c1, size of c1 must be 1536.
	* @remark, user must free the c1.
	* @return user must:
	* 	continue connect app if success,
	* 	try simple handshake if error is ERROR_RTMP_TRY_SIMPLE_HS,
	* 	otherwise, disconnect
	*/
	virtual int handshake_with_client(SrsSocket& skt, char* _c1);
	virtual int handshake_with_server(SrsSocket& skt);
};

/**
* auto free the instance in the current scope.
*/
#define SrsAutoFree(className, instance, is_array) \
	__SrsAutoFree<className> _auto_free_##instance(&instance, is_array)
    
template<class T>
class __SrsAutoFree
{
private:
    T** ptr;
    bool is_array;
public:
    /**
    * auto delete the ptr.
    * @is_array a bool value indicates whether the ptr is a array.
    */
    __SrsAutoFree(T** _ptr, bool _is_array){
        ptr = _ptr;
        is_array = _is_array;
    }
    
    virtual ~__SrsAutoFree(){
        if (ptr == NULL || *ptr == NULL) {
            return;
        }
        
        if (is_array) {
            delete[] *ptr;
        } else {
            delete *ptr;
        }
        
        *ptr = NULL;
    }
};

#include <string>
#include <vector>

class SrsStream;
class SrsAmf0Object;

/**
* any amf0 value.
* 2.1 Types Overview
* value-type = number-type | boolean-type | string-type | object-type 
* 		| null-marker | undefined-marker | reference-type | ecma-array-type 
* 		| strict-array-type | date-type | long-string-type | xml-document-type 
* 		| typed-object-type
*/
struct SrsAmf0Any
{
	char marker;

	SrsAmf0Any();
	virtual ~SrsAmf0Any();
	
	virtual bool is_string();
	virtual bool is_boolean();
	virtual bool is_number();
	virtual bool is_null();
	virtual bool is_undefined();
	virtual bool is_object();
	virtual bool is_object_eof();
	virtual bool is_ecma_array();
};

/**
* read amf0 string from stream.
* 2.4 String Type
* string-type = string-marker UTF-8
* @return default value is empty string.
*/
struct SrsAmf0String : public SrsAmf0Any
{
	std::string value;

	SrsAmf0String(const char* _value = NULL);
	virtual ~SrsAmf0String();
};

/**
* read amf0 boolean from stream.
* 2.4 String Type
* boolean-type = boolean-marker U8
* 		0 is false, <> 0 is true
* @return default value is false.
*/
struct SrsAmf0Boolean : public SrsAmf0Any
{
	bool value;

	SrsAmf0Boolean(bool _value = false);
	virtual ~SrsAmf0Boolean();
};

/**
* read amf0 number from stream.
* 2.2 Number Type
* number-type = number-marker DOUBLE
* @return default value is 0.
*/
struct SrsAmf0Number : public SrsAmf0Any
{
	double value;

	SrsAmf0Number(double _value = 0.0);
	virtual ~SrsAmf0Number();
};

/**
* read amf0 null from stream.
* 2.7 null Type
* null-type = null-marker
*/
struct SrsAmf0Null : public SrsAmf0Any
{
	SrsAmf0Null();
	virtual ~SrsAmf0Null();
};

/**
* read amf0 undefined from stream.
* 2.8 undefined Type
* undefined-type = undefined-marker
*/
struct SrsAmf0Undefined : public SrsAmf0Any
{
	SrsAmf0Undefined();
	virtual ~SrsAmf0Undefined();
};

/**
* 2.11 Object End Type
* object-end-type = UTF-8-empty object-end-marker
* 0x00 0x00 0x09
*/
struct SrsAmf0ObjectEOF : public SrsAmf0Any
{
	int16_t utf8_empty;

	SrsAmf0ObjectEOF();
	virtual ~SrsAmf0ObjectEOF();
};

/**
* to ensure in inserted order.
* for the FMLE will crash when AMF0Object is not ordered by inserted,
* if ordered in map, the string compare order, the FMLE will creash when
* get the response of connect app.
*/
struct SrsUnSortedHashtable
{
private:
	typedef std::pair<std::string, SrsAmf0Any*> SrsObjectPropertyType;
	std::vector<SrsObjectPropertyType> properties;
public:
	SrsUnSortedHashtable();
	virtual ~SrsUnSortedHashtable();
	
	virtual int size();
	virtual void clear();
	virtual std::string key_at(int index);
	virtual SrsAmf0Any* value_at(int index);
	virtual void set(std::string key, SrsAmf0Any* value);
	
	virtual SrsAmf0Any* get_property(std::string name);
	virtual SrsAmf0Any* ensure_property_string(std::string name);
	virtual SrsAmf0Any* ensure_property_number(std::string name);
};

/**
* 2.5 Object Type
* anonymous-object-type = object-marker *(object-property)
* object-property = (UTF-8 value-type) | (UTF-8-empty object-end-marker)
*/
struct SrsAmf0Object : public SrsAmf0Any
{
private:
	SrsUnSortedHashtable properties;
public:
	SrsAmf0ObjectEOF eof;

	SrsAmf0Object();
	virtual ~SrsAmf0Object();
	
	virtual int size();
	virtual std::string key_at(int index);
	virtual SrsAmf0Any* value_at(int index);
	virtual void set(std::string key, SrsAmf0Any* value);

	virtual SrsAmf0Any* get_property(std::string name);
	virtual SrsAmf0Any* ensure_property_string(std::string name);
	virtual SrsAmf0Any* ensure_property_number(std::string name);
};

/**
* 2.10 ECMA Array Type
* ecma-array-type = associative-count *(object-property)
* associative-count = U32
* object-property = (UTF-8 value-type) | (UTF-8-empty object-end-marker)
*/
struct SrsASrsAmf0EcmaArray : public SrsAmf0Any
{
private:
	SrsUnSortedHashtable properties;
public:
	int32_t count;
	SrsAmf0ObjectEOF eof;

	SrsASrsAmf0EcmaArray();
	virtual ~SrsASrsAmf0EcmaArray();
	
	virtual int size();
	virtual void clear();
	virtual std::string key_at(int index);
	virtual SrsAmf0Any* value_at(int index);
	virtual void set(std::string key, SrsAmf0Any* value);

	virtual SrsAmf0Any* get_property(std::string name);
	virtual SrsAmf0Any* ensure_property_string(std::string name);
};

/**
* read amf0 utf8 string from stream.
* 1.3.1 Strings and UTF-8
* UTF-8 = U16 *(UTF8-char)
* UTF8-char = UTF8-1 | UTF8-2 | UTF8-3 | UTF8-4
* UTF8-1 = %x00-7F
* @remark only support UTF8-1 char.
*/
extern int srs_amf0_read_utf8(SrsStream* stream, std::string& value);
extern int srs_amf0_write_utf8(SrsStream* stream, std::string value);

/**
* read amf0 string from stream.
* 2.4 String Type
* string-type = string-marker UTF-8
*/
extern int srs_amf0_read_string(SrsStream* stream, std::string& value);
extern int srs_amf0_write_string(SrsStream* stream, std::string value);

/**
* read amf0 boolean from stream.
* 2.4 String Type
* boolean-type = boolean-marker U8
* 		0 is false, <> 0 is true
*/
extern int srs_amf0_read_boolean(SrsStream* stream, bool& value);
extern int srs_amf0_write_boolean(SrsStream* stream, bool value);

/**
* read amf0 number from stream.
* 2.2 Number Type
* number-type = number-marker DOUBLE
*/
extern int srs_amf0_read_number(SrsStream* stream, double& value);
extern int srs_amf0_write_number(SrsStream* stream, double value);

/**
* read amf0 null from stream.
* 2.7 null Type
* null-type = null-marker
*/
extern int srs_amf0_read_null(SrsStream* stream);
extern int srs_amf0_write_null(SrsStream* stream);

/**
* read amf0 undefined from stream.
* 2.8 undefined Type
* undefined-type = undefined-marker
*/
extern int srs_amf0_read_undefined(SrsStream* stream);
extern int srs_amf0_write_undefined(SrsStream* stream);

extern int srs_amf0_read_any(SrsStream* stream, SrsAmf0Any*& value);

/**
* read amf0 object from stream.
* 2.5 Object Type
* anonymous-object-type = object-marker *(object-property)
* object-property = (UTF-8 value-type) | (UTF-8-empty object-end-marker)
*/
extern int srs_amf0_read_object(SrsStream* stream, SrsAmf0Object*& value);
extern int srs_amf0_write_object(SrsStream* stream, SrsAmf0Object* value);

/**
* read amf0 object from stream.
* 2.10 ECMA Array Type
* ecma-array-type = associative-count *(object-property)
* associative-count = U32
* object-property = (UTF-8 value-type) | (UTF-8-empty object-end-marker)
*/
extern int srs_amf0_read_ecma_array(SrsStream* stream, SrsASrsAmf0EcmaArray*& value);
extern int srs_amf0_write_ecma_array(SrsStream* stream, SrsASrsAmf0EcmaArray* value);

/**
* get amf0 objects size.
*/
extern int srs_amf0_get_utf8_size(std::string value);
extern int srs_amf0_get_string_size(std::string value);
extern int srs_amf0_get_number_size();
extern int srs_amf0_get_null_size();
extern int srs_amf0_get_undefined_size();
extern int srs_amf0_get_boolean_size();
extern int srs_amf0_get_object_size(SrsAmf0Object* obj);
extern int srs_amf0_get_ecma_array_size(SrsASrsAmf0EcmaArray* arr);

/**
* convert the any to specified object.
* @return T*, the converted object. never NULL.
* @remark, user must ensure the current object type, 
* 		or the covert will cause assert failed.
*/
template<class T>
T* srs_amf0_convert(SrsAmf0Any* any)
{
	T* p = dynamic_cast<T*>(any);
	srs_assert(p != NULL);
	return p;
}

/**
* the original request from client.
*/
struct SrsRequest
{
	/**
	* tcUrl: rtmp://request_vhost:port/app/stream
	* support pass vhost in query string, such as:
	*	rtmp://ip:port/app?vhost=request_vhost/stream
	*	rtmp://ip:port/app...vhost...request_vhost/stream
	*/
	std::string tcUrl;
	std::string pageUrl;
	std::string swfUrl;
	double objectEncoding;
	
	std::string schema;
	std::string vhost;
	std::string port;
	std::string app;
	std::string stream;
	
	SrsRequest();
	virtual ~SrsRequest();
#if 0 //disable the server-side behavior.
	/**
	* disconvery vhost/app from tcUrl.
	*/
	virtual int discovery_app();
#endif
	virtual std::string get_stream_url();
	virtual void strip();
private:
	std::string& trim(std::string& str, std::string chs);
};

/**
* the response to client.
*/
struct SrsResponse
{
	int stream_id;
	
	SrsResponse();
	virtual ~SrsResponse();
};

/**
* the rtmp client type.
*/
enum SrsClientType
{
	SrsClientUnknown,
	SrsClientPlay,
	SrsClientFMLEPublish,
	SrsClientFlashPublish,
};

class SrsSocket;
class SrsBuffer;
class SrsPacket;
class SrsStream;
class SrsCommonMessage;
class SrsChunkStream;
class SrsAmf0Object;
class SrsAmf0Null;
class SrsAmf0Undefined;
class ISrsMessage;

// convert class name to string.
#define CLASS_NAME_STRING(className) #className

/**
* max rtmp header size:
* 	1bytes basic header,
* 	11bytes message header,
* 	4bytes timestamp header,
* that is, 1+11+4=16bytes.
*/
#define RTMP_MAX_FMT0_HEADER_SIZE 16
/**
* max rtmp header size:
* 	1bytes basic header,
* 	4bytes timestamp header,
* that is, 1+4=5bytes.
*/
#define RTMP_MAX_FMT3_HEADER_SIZE 5

/**
* the protocol provides the rtmp-message-protocol services,
* to recv RTMP message from RTMP chunk stream,
* and to send out RTMP message over RTMP chunk stream.
*/
class SrsProtocol
{
private:
	struct AckWindowSize
	{
		int ack_window_size;
		int64_t acked_size;
		
		AckWindowSize();
	};
// peer in/out
private:
	st_netfd_t stfd;
	SrsSocket* skt;
	char* pp;
	/**
	* requests sent out, used to build the response.
	* key: transactionId
	* value: the request command name
	*/
	std::map<double, std::string> requests;
// peer in
private:
	std::map<int, SrsChunkStream*> chunk_streams;
	SrsBuffer* buffer;
	int32_t in_chunk_size;
	AckWindowSize in_ack_size;
// peer out
private:
	char out_header_fmt0[RTMP_MAX_FMT0_HEADER_SIZE];
	char out_header_fmt3[RTMP_MAX_FMT3_HEADER_SIZE];
	int32_t out_chunk_size;
public:
	SrsProtocol(st_netfd_t client_stfd);
	virtual ~SrsProtocol();
public:
	std::string get_request_name(double transcationId);
	/**
	* set the timeout in us.
	* if timeout, recv/send message return ERROR_SOCKET_TIMEOUT.
	*/
	virtual void set_recv_timeout(int64_t timeout_us);
	virtual int64_t get_recv_timeout();
	virtual void set_send_timeout(int64_t timeout_us);
	virtual int64_t get_recv_bytes();
	virtual int64_t get_send_bytes();
	virtual int get_recv_kbps();
	virtual int get_send_kbps();
	/**
	* recv a message with raw/undecoded payload from peer.
	* the payload is not decoded, use srs_rtmp_expect_message<T> if requires 
	* specifies message.
	* @pmsg, user must free it. NULL if not success.
	* @remark, only when success, user can use and must free the pmsg.
	*/
	virtual int recv_message(SrsCommonMessage** pmsg);
	/**
	* send out message with encoded payload to peer.
	* use the message encode method to encode to payload,
	* then sendout over socket.
	* @msg this method will free it whatever return value.
	*/
	virtual int send_message(ISrsMessage* msg);
private:
	/**
	* when recv message, update the context.
	*/
	virtual int on_recv_message(SrsCommonMessage* msg);
	virtual int response_acknowledgement_message();
	virtual int response_ping_message(int32_t timestamp);
	/**
	* when message sentout, update the context.
	*/
	virtual int on_send_message(ISrsMessage* msg);
	/**
	* try to recv interlaced message from peer,
	* return error if error occur and nerver set the pmsg,
	* return success and pmsg set to NULL if no entire message got,
	* return success and pmsg set to entire message if got one.
	*/
	virtual int recv_interlaced_message(SrsCommonMessage** pmsg);
	/**
	* read the chunk basic header(fmt, cid) from chunk stream.
	* user can discovery a SrsChunkStream by cid.
	* @bh_size return the chunk basic header size, to remove the used bytes when finished.
	*/
	virtual int read_basic_header(char& fmt, int& cid, int& bh_size);
	/**
	* read the chunk message header(timestamp, payload_length, message_type, stream_id) 
	* from chunk stream and save to SrsChunkStream.
	* @mh_size return the chunk message header size, to remove the used bytes when finished.
	*/
	virtual int read_message_header(SrsChunkStream* chunk, char fmt, int bh_size, int& mh_size);
	/**
	* read the chunk payload, remove the used bytes in buffer,
	* if got entire message, set the pmsg.
	* @payload_size read size in this roundtrip, generally a chunk size or left message size.
	*/
	virtual int read_message_payload(SrsChunkStream* chunk, int bh_size, int mh_size, int& payload_size, SrsCommonMessage** pmsg);
};

/**
* 4.1. Message Header
*/
struct SrsMessageHeader
{
	/**
	* One byte field to represent the message type. A range of type IDs
	* (1-7) are reserved for protocol control messages.
	*/
	int8_t message_type;
	/**
	* Three-byte field that represents the size of the payload in bytes.
	* It is set in big-endian format.
	*/
	int32_t payload_length;
	/**
	* Three-byte field that contains a timestamp delta of the message.
	* The 4 bytes are packed in the big-endian order.
	* @remark, only used for decoding message from chunk stream.
	*/
	int32_t timestamp_delta;
	/**
	* Three-byte field that identifies the stream of the message. These
	* bytes are set in big-endian format.
	*/
	int32_t stream_id;
	
	/**
	* Four-byte field that contains a timestamp of the message.
	* The 4 bytes are packed in the big-endian order.
	* @remark, used as calc timestamp when decode and encode time.
	*/
	u_int32_t timestamp;
	
	SrsMessageHeader();
	virtual ~SrsMessageHeader();

	bool is_audio();
	bool is_video();
	bool is_amf0_command();
	bool is_amf0_data();
	bool is_amf3_command();
	bool is_amf3_data();
	bool is_window_ackledgement_size();
	bool is_set_chunk_size();
	bool is_user_control_message();
};

/**
* incoming chunk stream maybe interlaced,
* use the chunk stream to cache the input RTMP chunk streams.
*/
class SrsChunkStream
{
public:
	/**
	* represents the basic header fmt,
	* which used to identify the variant message header type.
	*/
	char fmt;
	/**
	* represents the basic header cid,
	* which is the chunk stream id.
	*/
	int cid;
	/**
	* cached message header
	*/
	SrsMessageHeader header;
	/**
	* whether the chunk message header has extended timestamp.
	*/
	bool extended_timestamp;
	/**
	* partially read message.
	*/
	SrsCommonMessage* msg;
	/**
	* decoded msg count, to identify whether the chunk stream is fresh.
	*/
	int64_t msg_count;
public:
	SrsChunkStream(int _cid);
	virtual ~SrsChunkStream();
};

/**
* message to output.
*/
class ISrsMessage
{
// 4.1. Message Header
public:
	SrsMessageHeader header;
// 4.2. Message Payload
public:
	/**
	* The other part which is the payload is the actual data that is
	* contained in the message. For example, it could be some audio samples
	* or compressed video data. The payload format and interpretation are
	* beyond the scope of this document.
	*/
	int32_t size;
	int8_t* payload;
public:
	ISrsMessage();
	virtual ~ISrsMessage();
public:
	/**
	* whether message canbe decoded.
	* only update the context when message canbe decoded.
	*/
	virtual bool can_decode() = 0;
/**
* encode functions.
*/
public:
	/**
	* get the perfered cid(chunk stream id) which sendout over.
	*/
	virtual int get_perfer_cid() = 0;
	/**
	* encode the packet to message payload bytes.
	* @remark there exists empty packet, so maybe the payload is NULL.
	*/
	virtual int encode_packet() = 0;
};

/**
* common RTMP message defines in rtmp.part2.Message-Formats.pdf.
* cannbe parse and decode.
*/
class SrsCommonMessage : public ISrsMessage
{
private:
	typedef ISrsMessage super;
// decoded message payload.
private:
	SrsStream* stream;
	SrsPacket* packet;
public:
	SrsCommonMessage();
	virtual ~SrsCommonMessage();
public:
	virtual bool can_decode();
/**
* decode functions.
*/
public:
	/**
	* decode packet from message payload.
	*/
	// TODO: use protocol to decode it.
	virtual int decode_packet(SrsProtocol* protocol);
	/**
	* get the decoded packet which decoded by decode_packet().
	* @remark, user never free the pkt, the message will auto free it.
	*/
	virtual SrsPacket* get_packet();
/**
* encode functions.
*/
public:
	/**
	* get the perfered cid(chunk stream id) which sendout over.
	*/
	virtual int get_perfer_cid();
	/**
	* set the encoded packet to encode_packet() to payload.
	* @stream_id, the id of stream which is created by createStream.
	* @remark, user never free the pkt, the message will auto free it.
	*/
	// TODO: refine the send methods.
	virtual void set_packet(SrsPacket* pkt, int stream_id);
	/**
	* encode the packet to message payload bytes.
	* @remark there exists empty packet, so maybe the payload is NULL.
	*/
	virtual int encode_packet();
};

/**
* shared ptr message.
* for audio/video/data message that need less memory copy.
* and only for output.
*/
class SrsSharedPtrMessage : public ISrsMessage
{
private:
	typedef ISrsMessage super;
private:
	struct SrsSharedPtr
	{
		char* payload;
		int size;
		int perfer_cid;
		int shared_count;
		
		SrsSharedPtr();
		virtual ~SrsSharedPtr();
	};
	SrsSharedPtr* ptr;
public:
	SrsSharedPtrMessage();
	virtual ~SrsSharedPtrMessage();
public:
	virtual bool can_decode();
public:
	/**
	* set the shared payload.
	* we will detach the payload of source,
	* so ensure donot use it before.
	*/
	virtual int initialize(SrsCommonMessage* source);
	/**
	* set the shared payload.
	* we will use the payload, donot use the payload of source.
	*/
	virtual int initialize(SrsCommonMessage* source, char* payload, int size);
	virtual SrsSharedPtrMessage* copy();
public:
	/**
	* get the perfered cid(chunk stream id) which sendout over.
	*/
	virtual int get_perfer_cid();
	/**
	* ignored.
	* for shared message, nothing should be done.
	* use initialize() to set the data.
	*/
	virtual int encode_packet();
};

/**
* the decoded message payload.
* @remark we seperate the packet from message,
*		for the packet focus on logic and domain data,
*		the message bind to the protocol and focus on protocol, such as header.
* 		we can merge the message and packet, using OOAD hierachy, packet extends from message,
* 		it's better for me to use components -- the message use the packet as payload.
*/
class SrsPacket
{
protected:
	/**
	* subpacket must override to provide the right class name.
	*/
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsPacket);
	}
public:
	SrsPacket();
	virtual ~SrsPacket();
/**
* decode functions.
*/
public:
	/**
	* subpacket must override to decode packet from stream.
	* @remark never invoke the super.decode, it always failed.
	*/
	virtual int decode(SrsStream* stream);
/**
* encode functions.
*/
public:
	virtual int get_perfer_cid();
	virtual int get_payload_length();
public:
	/**
	* subpacket must override to provide the right message type.
	*/
	virtual int get_message_type();
	/**
	* the subpacket can override this encode,
	* for example, video and audio will directly set the payload withou memory copy,
	* other packet which need to serialize/encode to bytes by override the 
	* get_size and encode_packet.
	*/
	virtual int encode(int& size, char*& payload);
protected:
	/**
	* subpacket can override to calc the packet size.
	*/
	virtual int get_size();
	/**
	* subpacket can override to encode the payload to stream.
	* @remark never invoke the super.encode_packet, it always failed.
	*/
	virtual int encode_packet(SrsStream* stream);
};

/**
* 4.1.1. connect
* The client sends the connect command to the server to request
* connection to a server application instance.
*/
class SrsConnectAppPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsConnectAppPacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Object* command_object;
public:
	SrsConnectAppPacket();
	virtual ~SrsConnectAppPacket();
public:
	virtual int decode(SrsStream* stream);
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};
/**
* response for SrsConnectAppPacket.
*/
class SrsConnectAppResPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsConnectAppResPacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Object* props;
	SrsAmf0Object* info;
public:
	SrsConnectAppResPacket();
	virtual ~SrsConnectAppResPacket();
public:
	virtual int decode(SrsStream* stream);
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* 4.1.3. createStream
* The client sends this command to the server to create a logical
* channel for message communication The publishing of audio, video, and
* metadata is carried out over stream channel created using the
* createStream command.
*/
class SrsCreateStreamPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsCreateStreamPacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Null* command_object;
public:
	SrsCreateStreamPacket();
	virtual ~SrsCreateStreamPacket();
public:
	virtual int decode(SrsStream* stream);
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};
/**
* response for SrsCreateStreamPacket.
*/
class SrsCreateStreamResPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsCreateStreamResPacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Null* command_object;
	double stream_id;
public:
	SrsCreateStreamResPacket(double _transaction_id, double _stream_id);
	virtual ~SrsCreateStreamResPacket();
public:
	virtual int decode(SrsStream* stream);
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* FMLE start publish: ReleaseStream/PublishStream
*/
class SrsFMLEStartPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsFMLEStartPacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Null* command_object;
	std::string stream_name;
public:
	SrsFMLEStartPacket();
	virtual ~SrsFMLEStartPacket();
public:
	virtual int decode(SrsStream* stream);
};
/**
* response for SrsFMLEStartPacket.
*/
class SrsFMLEStartResPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsFMLEStartResPacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Null* command_object;
	SrsAmf0Undefined* args;
public:
	SrsFMLEStartResPacket(double _transaction_id);
	virtual ~SrsFMLEStartResPacket();
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* FMLE/flash publish
* 4.2.6. Publish
* The client sends the publish command to publish a named stream to the
* server. Using this name, any client can play this stream and receive
* the published audio, video, and data messages.
*/
class SrsPublishPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsPublishPacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Null* command_object;
	std::string stream_name;
	// optional, default to live.
	std::string type;
public:
	SrsPublishPacket();
	virtual ~SrsPublishPacket();
public:
	virtual int decode(SrsStream* stream);
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* 4.2.8. pause
* The client sends the pause command to tell the server to pause or
* start playing.
*/
class SrsPausePacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsPausePacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Null* command_object;
	bool is_pause;
	double time_ms;
public:
	SrsPausePacket();
	virtual ~SrsPausePacket();
public:
	virtual int decode(SrsStream* stream);
};

/**
* 4.2.1. play
* The client sends this command to the server to play a stream.
*/
class SrsPlayPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsPlayPacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Null* command_object;
	std::string stream_name;
	double start;
	double duration;
	bool reset;
public:
	SrsPlayPacket();
	virtual ~SrsPlayPacket();
public:
	virtual int decode(SrsStream* stream);
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};
/**
* response for SrsPlayPacket.
* @remark, user must set the stream_id in header.
*/
class SrsPlayResPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsPlayResPacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Null* command_object;
	SrsAmf0Object* desc;
public:
	SrsPlayResPacket();
	virtual ~SrsPlayResPacket();
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* when bandwidth test done, notice client.
*/
class SrsOnBWDonePacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsOnBWDonePacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Null* args;
public:
	SrsOnBWDonePacket();
	virtual ~SrsOnBWDonePacket();
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* onStatus command, AMF0 Call
* @remark, user must set the stream_id by SrsMessage.set_packet().
*/
class SrsOnStatusCallPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsOnStatusCallPacket);
	}
public:
	std::string command_name;
	double transaction_id;
	SrsAmf0Null* args;
	SrsAmf0Object* data;
public:
	SrsOnStatusCallPacket();
	virtual ~SrsOnStatusCallPacket();
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* onStatus data, AMF0 Data
* @remark, user must set the stream_id by SrsMessage.set_packet().
*/
class SrsOnStatusDataPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsOnStatusDataPacket);
	}
public:
	std::string command_name;
	SrsAmf0Object* data;
public:
	SrsOnStatusDataPacket();
	virtual ~SrsOnStatusDataPacket();
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* AMF0Data RtmpSampleAccess
* @remark, user must set the stream_id by SrsMessage.set_packet().
*/
class SrsSampleAccessPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsSampleAccessPacket);
	}
public:
	std::string command_name;
	bool video_sample_access;
	bool audio_sample_access;
public:
	SrsSampleAccessPacket();
	virtual ~SrsSampleAccessPacket();
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* the stream metadata.
* FMLE: @setDataFrame
* others: onMetaData
*/
class SrsOnMetaDataPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsOnMetaDataPacket);
	}
public:
	std::string name;
	SrsAmf0Object* metadata;
public:
	SrsOnMetaDataPacket();
	virtual ~SrsOnMetaDataPacket();
public:
	virtual int decode(SrsStream* stream);
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* 5.5. Window Acknowledgement Size (5)
* The client or the server sends this message to inform the peer which
* window size to use when sending acknowledgment.
*/
class SrsSetWindowAckSizePacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsSetWindowAckSizePacket);
	}
public:
	int32_t ackowledgement_window_size;
public:
	SrsSetWindowAckSizePacket();
	virtual ~SrsSetWindowAckSizePacket();
public:
	virtual int decode(SrsStream* stream);
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* 5.3. Acknowledgement (3)
* The client or the server sends the acknowledgment to the peer after
* receiving bytes equal to the window size.
*/
class SrsAcknowledgementPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsAcknowledgementPacket);
	}
public:
	int32_t sequence_number;
public:
	SrsAcknowledgementPacket();
	virtual ~SrsAcknowledgementPacket();
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* 7.1. Set Chunk Size
* Protocol control message 1, Set Chunk Size, is used to notify the
* peer about the new maximum chunk size.
*/
class SrsSetChunkSizePacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsSetChunkSizePacket);
	}
public:
	int32_t chunk_size;
public:
	SrsSetChunkSizePacket();
	virtual ~SrsSetChunkSizePacket();
public:
	virtual int decode(SrsStream* stream);
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* 5.6. Set Peer Bandwidth (6)
* The client or the server sends this message to update the output
* bandwidth of the peer.
*/
class SrsSetPeerBandwidthPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsSetPeerBandwidthPacket);
	}
public:
	int32_t bandwidth;
	int8_t type;
public:
	SrsSetPeerBandwidthPacket();
	virtual ~SrsSetPeerBandwidthPacket();
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

// 3.7. User Control message
enum SrcPCUCEventType
{
	 // generally, 4bytes event-data
	SrcPCUCStreamBegin 			= 0x00,
	SrcPCUCStreamEOF 			= 0x01,
	SrcPCUCStreamDry 			= 0x02,
	SrcPCUCSetBufferLength 		= 0x03, // 8bytes event-data
	SrcPCUCStreamIsRecorded 	= 0x04,
	SrcPCUCPingRequest 			= 0x06,
	SrcPCUCPingResponse 		= 0x07,
};

/**
* for the EventData is 4bytes.
* Stream Begin(=0)			4-bytes stream ID
* Stream EOF(=1)			4-bytes stream ID
* StreamDry(=2)				4-bytes stream ID
* SetBufferLength(=3)		8-bytes 4bytes stream ID, 4bytes buffer length.
* StreamIsRecorded(=4)		4-bytes stream ID
* PingRequest(=6)			4-bytes timestamp local server time
* PingResponse(=7)			4-bytes timestamp received ping request.
* 
* 3.7. User Control message
* +------------------------------+-------------------------
* | Event Type ( 2- bytes ) | Event Data
* +------------------------------+-------------------------
* Figure 5 Pay load for the ‘User Control Message’.
*/
class SrsUserControlPacket : public SrsPacket
{
private:
	typedef SrsPacket super;
protected:
	virtual const char* get_class_name()
	{
		return CLASS_NAME_STRING(SrsUserControlPacket);
	}
public:
	// @see: SrcPCUCEventType
	int16_t event_type;
	int32_t event_data;
	/**
	* 4bytes if event_type is SetBufferLength; otherwise 0.
	*/
	int32_t extra_data;
public:
	SrsUserControlPacket();
	virtual ~SrsUserControlPacket();
public:
	virtual int decode(SrsStream* stream);
public:
	virtual int get_perfer_cid();
public:
	virtual int get_message_type();
protected:
	virtual int get_size();
	virtual int encode_packet(SrsStream* stream);
};

/**
* expect a specified message, drop others util got specified one.
* @pmsg, user must free it. NULL if not success.
* @ppacket, store in the pmsg, user must never free it. NULL if not success.
* @remark, only when success, user can use and must free the pmsg/ppacket.
*/
template<class T>
int srs_rtmp_expect_message(SrsProtocol* protocol, SrsCommonMessage** pmsg, T** ppacket)
{
	*pmsg = NULL;
	*ppacket = NULL;
	
	int ret = ERROR_SUCCESS;
	
	while (true) {
		SrsCommonMessage* msg = NULL;
		if ((ret = protocol->recv_message(&msg)) != ERROR_SUCCESS) {
			srs_error("recv message failed. ret=%d", ret);
			return ret;
		}
		srs_verbose("recv message success.");
		
		if ((ret = msg->decode_packet(protocol)) != ERROR_SUCCESS) {
			delete msg;
			srs_error("decode message failed. ret=%d", ret);
			return ret;
		}
		
		T* pkt = dynamic_cast<T*>(msg->get_packet());
		if (!pkt) {
			delete msg;
			srs_trace("drop message(type=%d, size=%d, time=%d, sid=%d).", 
				msg->header.message_type, msg->header.payload_length,
				msg->header.timestamp, msg->header.stream_id);
			continue;
		}
		
		*pmsg = msg;
		*ppacket = pkt;
		break;
	}
	
	return ret;
}

/**
* implements the client role protocol.
*/
class SrsRtmpClient
{
private:
	SrsProtocol* protocol;
	st_netfd_t stfd;
public:
	SrsRtmpClient(st_netfd_t _stfd);
	virtual ~SrsRtmpClient();
public:
	virtual void set_recv_timeout(int64_t timeout_us);
	virtual void set_send_timeout(int64_t timeout_us);
	virtual int64_t get_recv_bytes();
	virtual int64_t get_send_bytes();
	virtual int get_recv_kbps();
	virtual int get_send_kbps();
	virtual int recv_message(SrsCommonMessage** pmsg);
	virtual int send_message(ISrsMessage* msg);
public:
	virtual int handshake();
	virtual int connect_app(std::string app, std::string tc_url);
	virtual int create_stream(int& stream_id);
	virtual int play(std::string stream, int stream_id);
	virtual int publish(std::string stream, int stream_id);
};

#endif
