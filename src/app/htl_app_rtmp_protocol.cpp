#include <htl_stdinc.hpp>

#include <inttypes.h>
#include <assert.h>
#include <sys/time.h>
#include <stdlib.h>

#include <string>
#include <sstream>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>

#include <htl_app_rtmp_protocol.hpp>

int64_t srs_get_system_time_ms()
{
    timeval now;
    
    gettimeofday(&now, NULL);

    // we must convert the tv_sec/tv_usec to int64_t.
    int64_t _srs_system_time_us_cache = now.tv_sec * 1000 * 1000 + now.tv_usec;
    
    _srs_system_time_us_cache = srs_max(0, _srs_system_time_us_cache);
    
    return _srs_system_time_us_cache;
}

#define SOCKET_READ_SIZE 4096

SrsBuffer::SrsBuffer()
{
}

SrsBuffer::~SrsBuffer()
{
}

int SrsBuffer::size()
{
	return (int)data.size();
}

char* SrsBuffer::bytes()
{
	return &data.at(0);
}

void SrsBuffer::erase(int size)
{
	data.erase(data.begin(), data.begin() + size);
}

void SrsBuffer::append(char* bytes, int size)
{
	data.insert(data.end(), bytes, bytes + size);
}

int SrsBuffer::ensure_buffer_bytes(SrsSocket* skt, int required_size)
{
	int ret = ERROR_SUCCESS;

	if (required_size < 0) {
		ret = ERROR_SYSTEM_SIZE_NEGATIVE;
		srs_error("size is negative. size=%d, ret=%d", required_size, ret);
		return ret;
	}

	while (size() < required_size) {
		char buffer[SOCKET_READ_SIZE];
		
		ssize_t nread;
		if ((ret = skt->read(buffer, SOCKET_READ_SIZE, &nread)) != ERROR_SUCCESS) {
			return ret;
		}
		
		srs_assert((int)nread > 0);
		append(buffer, (int)nread);
	}
	
	return ret;
}

SrsStream::SrsStream()
{
	p = bytes = NULL;
	size = 0;
}

SrsStream::~SrsStream()
{
}

int SrsStream::initialize(char* _bytes, int _size)
{
	int ret = ERROR_SUCCESS;
	
	if (!_bytes) {
		ret = ERROR_SYSTEM_STREAM_INIT;
		srs_error("stream param bytes must not be NULL. ret=%d", ret);
		return ret;
	}
	
	if (_size <= 0) {
		ret = ERROR_SYSTEM_STREAM_INIT;
		srs_error("stream param size must be positive. ret=%d", ret);
		return ret;
	}

	size = _size;
	p = bytes = _bytes;

	return ret;
}

void SrsStream::reset()
{
	p = bytes;
}

bool SrsStream::empty()
{
	return !p || !bytes || (p >= bytes + size);
}

bool SrsStream::require(int required_size)
{
	return !empty() && (required_size <= bytes + size - p);
}

void SrsStream::skip(int size)
{
	p += size;
}

int SrsStream::pos()
{
	if (empty()) {
		return 0;
	}
	
	return p - bytes;
}

int SrsStream::left()
{
	return size - pos();
}

char* SrsStream::current()
{
	return p;
}

int8_t SrsStream::read_1bytes()
{
	srs_assert(require(1));
	
	return (int8_t)*p++;
}

int16_t SrsStream::read_2bytes()
{
	srs_assert(require(2));
	
	int16_t value;
	pp = (char*)&value;
	pp[1] = *p++;
	pp[0] = *p++;
	
	return value;
}

int32_t SrsStream::read_3bytes()
{
	srs_assert(require(3));
	
	int32_t value = 0x00;
	pp = (char*)&value;
	pp[2] = *p++;
	pp[1] = *p++;
	pp[0] = *p++;
	
	return value;
}

int32_t SrsStream::read_4bytes()
{
	srs_assert(require(4));
	
	int32_t value;
	pp = (char*)&value;
	pp[3] = *p++;
	pp[2] = *p++;
	pp[1] = *p++;
	pp[0] = *p++;
	
	return value;
}

int64_t SrsStream::read_8bytes()
{
	srs_assert(require(8));
	
	int64_t value;
	pp = (char*)&value;
    pp[7] = *p++;
    pp[6] = *p++;
    pp[5] = *p++;
    pp[4] = *p++;
    pp[3] = *p++;
    pp[2] = *p++;
    pp[1] = *p++;
    pp[0] = *p++;
	
	return value;
}

std::string SrsStream::read_string(int len)
{
	srs_assert(require(len));
	
	std::string value;
	value.append(p, len);
	
	p += len;
	
	return value;
}

void SrsStream::write_1bytes(int8_t value)
{
	srs_assert(require(1));
	
	*p++ = value;
}

void SrsStream::write_2bytes(int16_t value)
{
	srs_assert(require(2));
	
	pp = (char*)&value;
	*p++ = pp[1];
	*p++ = pp[0];
}

void SrsStream::write_4bytes(int32_t value)
{
	srs_assert(require(4));
	
	pp = (char*)&value;
	*p++ = pp[3];
	*p++ = pp[2];
	*p++ = pp[1];
	*p++ = pp[0];
}

void SrsStream::write_8bytes(int64_t value)
{
	srs_assert(require(8));
	
	pp = (char*)&value;
	*p++ = pp[7];
	*p++ = pp[6];
	*p++ = pp[5];
	*p++ = pp[4];
	*p++ = pp[3];
	*p++ = pp[2];
	*p++ = pp[1];
	*p++ = pp[0];
}

void SrsStream::write_string(std::string value)
{
	srs_assert(require(value.length()));
	
	memcpy(p, value.data(), value.length());
	p += value.length();
}

SrsSocket::SrsSocket(st_netfd_t client_stfd)
{
    stfd = client_stfd;
	send_timeout = recv_timeout = ST_UTIME_NO_TIMEOUT;
	recv_bytes = send_bytes = 0;
	start_time_ms = srs_get_system_time_ms();
}

SrsSocket::~SrsSocket()
{
}

void SrsSocket::set_recv_timeout(int64_t timeout_us)
{
	recv_timeout = timeout_us;
}

int64_t SrsSocket::get_recv_timeout()
{
	return recv_timeout;
}

void SrsSocket::set_send_timeout(int64_t timeout_us)
{
	send_timeout = timeout_us;
}

int64_t SrsSocket::get_recv_bytes()
{
	return recv_bytes;
}

int64_t SrsSocket::get_send_bytes()
{
	return send_bytes;
}

int SrsSocket::get_recv_kbps()
{
	int64_t diff_ms = srs_get_system_time_ms() - start_time_ms;
	
	if (diff_ms <= 0) {
		return 0;
	}
	
	return recv_bytes * 8 / diff_ms;
}

int SrsSocket::get_send_kbps()
{
	int64_t diff_ms = srs_get_system_time_ms() - start_time_ms;
	
	if (diff_ms <= 0) {
		return 0;
	}
	
	return send_bytes * 8 / diff_ms;
}

int SrsSocket::read(const void* buf, size_t size, ssize_t* nread)
{
    int ret = ERROR_SUCCESS;
    
    *nread = st_read(stfd, (void*)buf, size, recv_timeout);
    
    // On success a non-negative integer indicating the number of bytes actually read is returned 
    // (a value of 0 means the network connection is closed or end of file is reached).
    if (*nread <= 0) {
		if (errno == ETIME) {
			return ERROR_SOCKET_TIMEOUT;
		}
		
        if (*nread == 0) {
            errno = ECONNRESET;
        }
        
        return ERROR_SOCKET_READ;
    }
    
    recv_bytes += *nread;
        
    return ret;
}

int SrsSocket::read_fully(const void* buf, size_t size, ssize_t* nread)
{
    int ret = ERROR_SUCCESS;
    
    *nread = st_read_fully(stfd, (void*)buf, size, recv_timeout);
    
    // On success a non-negative integer indicating the number of bytes actually read is returned 
    // (a value less than nbyte means the network connection is closed or end of file is reached)
    if (*nread != (ssize_t)size) {
		if (errno == ETIME) {
			return ERROR_SOCKET_TIMEOUT;
		}
		
        if (*nread >= 0) {
            errno = ECONNRESET;
        }
        
        return ERROR_SOCKET_READ_FULLY;
    }
    
    recv_bytes += *nread;
    
    return ret;
}

int SrsSocket::write(const void* buf, size_t size, ssize_t* nwrite)
{
    int ret = ERROR_SUCCESS;
    
    *nwrite = st_write(stfd, (void*)buf, size, send_timeout);
    
    if (*nwrite <= 0) {
		if (errno == ETIME) {
			return ERROR_SOCKET_TIMEOUT;
		}
		
        return ERROR_SOCKET_WRITE;
    }
    
    send_bytes += *nwrite;
        
    return ret;
}

int SrsSocket::writev(const iovec *iov, int iov_size, ssize_t* nwrite)
{
    int ret = ERROR_SUCCESS;
    
    *nwrite = st_writev(stfd, iov, iov_size, send_timeout);
    
    if (*nwrite <= 0) {
		if (errno == ETIME) {
			return ERROR_SOCKET_TIMEOUT;
		}
		
        return ERROR_SOCKET_WRITE;
    }
    
    send_bytes += *nwrite;
    
    return ret;
}

void srs_random_generate(char* bytes, int size)
{
	static char cdata[]  = { 
		0x73, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x2d, 0x72, 0x74, 0x6d, 0x70, 0x2d, 0x73, 0x65, 
		0x72, 0x76, 0x65, 0x72, 0x2d, 0x77, 0x69, 0x6e, 0x6c, 0x69, 0x6e, 0x2d, 0x77, 0x69, 
		0x6e, 0x74, 0x65, 0x72, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x40, 0x31, 0x32, 0x36, 
		0x2e, 0x63, 0x6f, 0x6d
	};
	for (int i = 0; i < size; i++) {
		bytes[i] = cdata[rand() % (sizeof(cdata) - 1)];
	}
}

#ifdef SRS_SSL

// 68bytes FMS key which is used to sign the sever packet.
u_int8_t SrsGenuineFMSKey[] = {
    0x47, 0x65, 0x6e, 0x75, 0x69, 0x6e, 0x65, 0x20,
    0x41, 0x64, 0x6f, 0x62, 0x65, 0x20, 0x46, 0x6c,
    0x61, 0x73, 0x68, 0x20, 0x4d, 0x65, 0x64, 0x69,
    0x61, 0x20, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72,
    0x20, 0x30, 0x30, 0x31, // Genuine Adobe Flash Media Server 001
    0xf0, 0xee, 0xc2, 0x4a, 0x80, 0x68, 0xbe, 0xe8,
    0x2e, 0x00, 0xd0, 0xd1, 0x02, 0x9e, 0x7e, 0x57,
    0x6e, 0xec, 0x5d, 0x2d, 0x29, 0x80, 0x6f, 0xab,
    0x93, 0xb8, 0xe6, 0x36, 0xcf, 0xeb, 0x31, 0xae
}; // 68

// 62bytes FP key which is used to sign the client packet.
u_int8_t SrsGenuineFPKey[] = {
    0x47, 0x65, 0x6E, 0x75, 0x69, 0x6E, 0x65, 0x20,
    0x41, 0x64, 0x6F, 0x62, 0x65, 0x20, 0x46, 0x6C,
    0x61, 0x73, 0x68, 0x20, 0x50, 0x6C, 0x61, 0x79,
    0x65, 0x72, 0x20, 0x30, 0x30, 0x31, // Genuine Adobe Flash Player 001
    0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8,
    0x2E, 0x00, 0xD0, 0xD1, 0x02, 0x9E, 0x7E, 0x57,
    0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB,
    0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
}; // 62

#include <openssl/evp.h>
#include <openssl/hmac.h>
int openssl_HMACsha256(const void* data, int data_size, const void* key, int key_size, void* digest) {
    HMAC_CTX ctx;
    
    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, (unsigned char*) key, key_size, EVP_sha256(), NULL);
    HMAC_Update(&ctx, (unsigned char *) data, data_size);

    unsigned int digest_size;
    HMAC_Final(&ctx, (unsigned char *) digest, &digest_size);
    
    HMAC_CTX_cleanup(&ctx);
    
    if (digest_size != 32) {
        return ERROR_OpenSslSha256DigestSize;
    }
    
    return ERROR_SUCCESS;
}

#include <openssl/dh.h>
#define RFC2409_PRIME_1024 \
        "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1" \
        "29024E088A67CC74020BBEA63B139B22514A08798E3404DD" \
        "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245" \
        "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED" \
        "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381" \
        "FFFFFFFFFFFFFFFF"
int __openssl_generate_key(
	u_int8_t*& _private_key, u_int8_t*& _public_key, int32_t& size,
	DH*& pdh, int32_t& bits_count, u_int8_t*& shared_key, int32_t& shared_key_length, BIGNUM*& peer_public_key
){
	int ret = ERROR_SUCCESS;

    //1. Create the DH
    if ((pdh = DH_new()) == NULL) {
        ret = ERROR_OpenSslCreateDH; 
        return ret;
    }

    //2. Create his internal p and g
    if ((pdh->p = BN_new()) == NULL) {
        ret = ERROR_OpenSslCreateP; 
        return ret;
    }
    if ((pdh->g = BN_new()) == NULL) {
        ret = ERROR_OpenSslCreateG; 
        return ret;
    }

    //3. initialize p, g and key length
    if (BN_hex2bn(&pdh->p, RFC2409_PRIME_1024) == 0) {
        ret = ERROR_OpenSslParseP1024; 
        return ret;
    }
    if (BN_set_word(pdh->g, 2) != 1) {
        ret = ERROR_OpenSslSetG;
        return ret;
    }

    //4. Set the key length
    pdh->length = bits_count;

    //5. Generate private and public key
    if (DH_generate_key(pdh) != 1) {
        ret = ERROR_OpenSslGenerateDHKeys;
        return ret;
    }

    // CreateSharedKey
    if (pdh == NULL) {
        ret = ERROR_OpenSslGenerateDHKeys;
        return ret;
    }

    if (shared_key_length != 0 || shared_key != NULL) {
        ret = ERROR_OpenSslShareKeyComputed;
        return ret;
    }

    shared_key_length = DH_size(pdh);
    if (shared_key_length <= 0 || shared_key_length > 1024) {
        ret = ERROR_OpenSslGetSharedKeySize;
        return ret;
    }
    shared_key = new u_int8_t[shared_key_length];
    memset(shared_key, 0, shared_key_length);

    peer_public_key = BN_bin2bn(_private_key, size, 0);
    if (peer_public_key == NULL) {
        ret = ERROR_OpenSslGetPeerPublicKey;
        return ret;
    }

    if (DH_compute_key(shared_key, peer_public_key, pdh) == -1) {
        ret = ERROR_OpenSslComputeSharedKey;
        return ret;
    }

    // CopyPublicKey
    if (pdh == NULL) {
        ret = ERROR_OpenSslComputeSharedKey;
		return ret;
    }
    
    int32_t keySize = BN_num_bytes(pdh->pub_key);
    if ((keySize <= 0) || (size <= 0) || (keySize > size)) {
        //("CopyPublicKey failed due to either invalid DH state or invalid call"); return ret;
        ret = ERROR_OpenSslInvalidDHState; 
        return ret;
    }

    if (BN_bn2bin(pdh->pub_key, _public_key) != keySize) {
        //("Unable to copy key"); return ret;
        ret = ERROR_OpenSslCopyKey; 
        return ret;
    }
    
    return ret;
}
int openssl_generate_key(char* _private_key, char* _public_key, int32_t size)
{
    int ret = ERROR_SUCCESS;

    // Initialize
    DH* pdh = NULL;
    int32_t bits_count = 1024; 
    u_int8_t* shared_key = NULL;
    int32_t shared_key_length = 0;
    BIGNUM* peer_public_key = NULL;
    
    ret = __openssl_generate_key(
    	(u_int8_t*&)_private_key, (u_int8_t*&)_public_key, size,
    	pdh, bits_count, shared_key, shared_key_length, peer_public_key
    );
    
    if (pdh != NULL) {
        if (pdh->p != NULL) {
            BN_free(pdh->p);
            pdh->p = NULL;
        }
        if (pdh->g != NULL) {
            BN_free(pdh->g);
            pdh->g = NULL;
        }
        DH_free(pdh);
        pdh = NULL;
    }

    if (shared_key != NULL) {
        delete[] shared_key;
        shared_key = NULL;
    }

    if (peer_public_key != NULL) {
        BN_free(peer_public_key);
        peer_public_key = NULL;
    }

    return ret;
}

// the digest key generate size.
#define OpensslHashSize 512

/**
* 764bytes key结构
* 	random-data: (offset)bytes
* 	key-data: 128bytes
* 	random-data: (764-offset-128-4)bytes
* 	offset: 4bytes
*/
struct key_block
{
	// (offset)bytes
	char* random0;
	int random0_size;
	
	// 128bytes
	char key[128];
	
	// (764-offset-128-4)bytes
	char* random1;
	int random1_size;
	
	// 4bytes
	int32_t offset;
};
// calc the offset of key,
// the key->offset cannot be used as the offset of key.
int srs_key_block_get_offset(key_block* key)
{
	int max_offset_size = 764 - 128 - 4;
	
	int offset = 0;
	u_int8_t* pp = (u_int8_t*)&key->offset;
	offset += *pp++;
	offset += *pp++;
	offset += *pp++;
	offset += *pp++;

	return offset % max_offset_size;
}
// create new key block data.
// if created, user must free it by srs_key_block_free
void srs_key_block_init(key_block* key)
{
	key->offset = (int32_t)rand();
	key->random0 = NULL;
	key->random1 = NULL;
	
	int offset = srs_key_block_get_offset(key);
	srs_assert(offset >= 0);
	
	key->random0_size = offset;
	if (key->random0_size > 0) {
		key->random0 = new char[key->random0_size];
		srs_random_generate(key->random0, key->random0_size);
	}
	
	srs_random_generate(key->key, sizeof(key->key));
	
	key->random1_size = 764 - offset - 128 - 4;
	if (key->random1_size > 0) {
		key->random1 = new char[key->random1_size];
		srs_random_generate(key->random1, key->random1_size);
	}
}
// parse key block from c1s1.
// if created, user must free it by srs_key_block_free
// @c1s1_key_bytes the key start bytes, maybe c1s1 or c1s1+764
int srs_key_block_parse(key_block* key, char* c1s1_key_bytes)
{
	int ret = ERROR_SUCCESS;

	char* pp = c1s1_key_bytes + 764;
	
	pp -= sizeof(int32_t);
	key->offset = *(int32_t*)pp;
	
	key->random0 = NULL;
	key->random1 = NULL;
	
	int offset = srs_key_block_get_offset(key);
	srs_assert(offset >= 0);
	
	pp = c1s1_key_bytes;
	key->random0_size = offset;
	if (key->random0_size > 0) {
		key->random0 = new char[key->random0_size];
		memcpy(key->random0, pp, key->random0_size);
	}
	pp += key->random0_size;
	
	memcpy(key->key, pp, sizeof(key->key));
	pp += sizeof(key->key);
	
	key->random1_size = 764 - offset - 128 - 4;
	if (key->random1_size > 0) {
		key->random1 = new char[key->random1_size];
		memcpy(key->random1, pp, key->random1_size);
	}
	
	return ret;
}
// free the block data create by 
// srs_key_block_init or srs_key_block_parse
void srs_key_block_free(key_block* key)
{
	if (key->random0) {
		srs_freepa(key->random0);
	}
	if (key->random1) {
		srs_freepa(key->random1);
	}
}

/**
* 764bytes digest结构
* 	offset: 4bytes
* 	random-data: (offset)bytes
* 	digest-data: 32bytes
* 	random-data: (764-4-offset-32)bytes
*/
struct digest_block
{
	// 4bytes
	int32_t offset;
	
	// (offset)bytes
	char* random0;
	int random0_size;
	
	// 32bytes
	char digest[32];
	
	// (764-4-offset-32)bytes
	char* random1;
	int random1_size;
};
// calc the offset of digest,
// the key->offset cannot be used as the offset of digest.
int srs_digest_block_get_offset(digest_block* digest)
{
	int max_offset_size = 764 - 32 - 4;
	
	int offset = 0;
	u_int8_t* pp = (u_int8_t*)&digest->offset;
	offset += *pp++;
	offset += *pp++;
	offset += *pp++;
	offset += *pp++;

	return offset % max_offset_size;
}
// create new digest block data.
// if created, user must free it by srs_digest_block_free
void srs_digest_block_init(digest_block* digest)
{
	digest->offset = (int32_t)rand();
	digest->random0 = NULL;
	digest->random1 = NULL;
	
	int offset = srs_digest_block_get_offset(digest);
	srs_assert(offset >= 0);
	
	digest->random0_size = offset;
	if (digest->random0_size > 0) {
		digest->random0 = new char[digest->random0_size];
		srs_random_generate(digest->random0, digest->random0_size);
	}
	
	srs_random_generate(digest->digest, sizeof(digest->digest));
	
	digest->random1_size = 764 - 4 - offset - 32;
	if (digest->random1_size > 0) {
		digest->random1 = new char[digest->random1_size];
		srs_random_generate(digest->random1, digest->random1_size);
	}
}
// parse digest block from c1s1.
// if created, user must free it by srs_digest_block_free
// @c1s1_digest_bytes the digest start bytes, maybe c1s1 or c1s1+764
int srs_digest_block_parse(digest_block* digest, char* c1s1_digest_bytes)
{
	int ret = ERROR_SUCCESS;

	char* pp = c1s1_digest_bytes;
	
	digest->offset = *(int32_t*)pp;
	pp += sizeof(int32_t);
	
	digest->random0 = NULL;
	digest->random1 = NULL;
	
	int offset = srs_digest_block_get_offset(digest);
	srs_assert(offset >= 0);
	
	digest->random0_size = offset;
	if (digest->random0_size > 0) {
		digest->random0 = new char[digest->random0_size];
		memcpy(digest->random0, pp, digest->random0_size);
	}
	pp += digest->random0_size;
	
	memcpy(digest->digest, pp, sizeof(digest->digest));
	pp += sizeof(digest->digest);
	
	digest->random1_size = 764 - 4 - offset - 32;
	if (digest->random1_size > 0) {
		digest->random1 = new char[digest->random1_size];
		memcpy(digest->random1, pp, digest->random1_size);
	}
	
	return ret;
}
// free the block data create by 
// srs_digest_block_init or srs_digest_block_parse
void srs_digest_block_free(digest_block* digest)
{
	if (digest->random0) {
		srs_freepa(digest->random0);
	}
	if (digest->random1) {
		srs_freepa(digest->random1);
	}
}

/**
* the schema type.
*/
enum srs_schema_type {
	srs_schema0 = 0, // key-digest sequence
	srs_schema1 = 1, // digest-key sequence
	srs_schema_invalid = 2,
};

void __srs_time_copy_to(char*& pp, int32_t time)
{
	// 4bytes time
	*(int32_t*)pp = time;
	pp += 4;
}
void __srs_version_copy_to(char*& pp, int32_t version)
{
	// 4bytes version
	*(int32_t*)pp = version;
	pp += 4;
}
void __srs_key_copy_to(char*& pp, key_block* key)
{
	// 764bytes key block
	if (key->random0_size > 0) {
		memcpy(pp, key->random0, key->random0_size);
	}
	pp += key->random0_size;
	
	memcpy(pp, key->key, sizeof(key->key));
	pp += sizeof(key->key);
	
	if (key->random1_size > 0) {
		memcpy(pp, key->random1, key->random1_size);
	}
	pp += key->random1_size;
	
	*(int32_t*)pp = key->offset;
	pp += 4;
}
void __srs_digest_copy_to(char*& pp, digest_block* digest, bool with_digest)
{
	// 732bytes digest block without the 32bytes digest-data
	// nbytes digest block part1
	*(int32_t*)pp = digest->offset;
	pp += 4;
	
	if (digest->random0_size > 0) {
		memcpy(pp, digest->random0, digest->random0_size);
	}
	pp += digest->random0_size;
	
	// digest
	if (with_digest) {
		memcpy(pp, digest->digest, 32);
		pp += 32;
	}
	
	// nbytes digest block part2
	if (digest->random1_size > 0) {
		memcpy(pp, digest->random1, digest->random1_size);
	}
	pp += digest->random1_size;
}

/**
* copy whole c1s1 to bytes.
*/
void srs_schema0_copy_to(char* bytes, bool with_digest, 
	int32_t time, int32_t version, key_block* key, digest_block* digest)
{
	char* pp = bytes;

	__srs_time_copy_to(pp, time);
	__srs_version_copy_to(pp, version);
	__srs_key_copy_to(pp, key);
	__srs_digest_copy_to(pp, digest, with_digest);
	
	if (with_digest) {
		srs_assert(pp - bytes == 1536);
	} else {
		srs_assert(pp - bytes == 1536 - 32);
	}
}
void srs_schema1_copy_to(char* bytes, bool with_digest, 
	int32_t time, int32_t version, digest_block* digest, key_block* key)
{
	char* pp = bytes;

	__srs_time_copy_to(pp, time);
	__srs_version_copy_to(pp, version);
	__srs_digest_copy_to(pp, digest, with_digest);
	__srs_key_copy_to(pp, key);
	
	if (with_digest) {
		srs_assert(pp - bytes == 1536);
	} else {
		srs_assert(pp - bytes == 1536 - 32);
	}
}
/**
* c1s1 is splited by digest:
* 	c1s1-part1: n bytes (time, version, key and digest-part1).
* 	digest-data: 32bytes
* 	c1s1-part2: (1536-n-32)bytes (digest-part2)
*/
char* srs_bytes_join_schema0(int32_t time, int32_t version, key_block* key, digest_block* digest)
{
	char* bytes = new char[1536 -32];
	
	srs_schema0_copy_to(bytes, false, time, version, key, digest);
	
	return bytes;
}
/**
* c1s1 is splited by digest:
* 	c1s1-part1: n bytes (time, version and digest-part1).
* 	digest-data: 32bytes
* 	c1s1-part2: (1536-n-32)bytes (digest-part2 and key)
*/
char* srs_bytes_join_schema1(int32_t time, int32_t version, digest_block* digest, key_block* key)
{
	char* bytes = new char[1536 -32];
	
	srs_schema1_copy_to(bytes, false, time, version, digest, key);
	
	return bytes;
}

/**
* compare the memory in bytes.
*/
bool srs_bytes_equals(void* pa, void* pb, int size){
	u_int8_t* a = (u_int8_t*)pa;
	u_int8_t* b = (u_int8_t*)pb;
	
    for(int i = 0; i < size; i++){
        if(a[i] != b[i]){
            return false;
        }
    }

    return true;
}

/**
* c1s1 schema0
* 	time: 4bytes
* 	version: 4bytes
* 	key: 764bytes
* 	digest: 764bytes
* c1s1 schema1
* 	time: 4bytes
* 	version: 4bytes
* 	digest: 764bytes
* 	key: 764bytes
*/
struct c1s1
{
	union block {
		key_block key; 
		digest_block digest; 
	};
	
	// 4bytes
	int32_t time;
	// 4bytes
	int32_t version;
	// 764bytes
	// if schema0, use key
	// if schema1, use digest
	block block0;
	// 764bytes
	// if schema0, use digest
	// if schema1, use key
	block block1;
	
	// the logic schema
	srs_schema_type schema;
	
	c1s1();
	virtual ~c1s1();
	/**
	* get the digest key.
	*/
	virtual char* get_digest();
	/**
	* copy to bytes.
	*/
	virtual void dump(char* _c1s1);
	
	/**
	* client: create and sign c1 by schema.
	* sign the c1, generate the digest.
	* 		calc_c1_digest(c1, schema) {
    *			get c1s1-joined from c1 by specified schema
    *			digest-data = HMACsha256(c1s1-joined, FPKey, 30)
    *			return digest-data;
	*		}
	*		random fill 1536bytes c1 // also fill the c1-128bytes-key
	*		time = time() // c1[0-3]
	*		version = [0x80, 0x00, 0x07, 0x02] // c1[4-7]
	*		schema = choose schema0 or schema1
	*		digest-data = calc_c1_digest(c1, schema)
	*		copy digest-data to c1
	*/
	virtual int c1_create(srs_schema_type _schema);
	/**
	* server: parse the c1s1, discovery the key and digest by schema.
	* use the c1_validate_digest() to valid the digest of c1.
	*/
	virtual int c1_parse(char* _c1s1, srs_schema_type _schema);
	/**
	* server: validate the parsed schema and c1s1
	*/
	virtual int c1_validate_digest(bool& is_valid);
	/**
	* server: create and sign the s1 from c1.
	*/
	virtual int s1_create(c1s1* c1);
private:
	virtual int calc_s1_digest(char*& digest);
	virtual int calc_c1_digest(char*& digest);
	virtual void destroy_blocks();
};

/**
* the c2s2 complex handshake structure.
* random-data: 1504bytes
* digest-data: 32bytes
*/
struct c2s2
{
	char random[1504];
	char digest[32];
	
	c2s2();
	virtual ~c2s2();
	
	/**
	* copy to bytes.
	*/
	virtual void dump(char* _c2s2);

	/**
	* create c2.
	* random fill c2s2 1536 bytes
	* 
	* // client generate C2, or server valid C2
	* temp-key = HMACsha256(s1-digest, FPKey, 62)
	* c2-digest-data = HMACsha256(c2-random-data, temp-key, 32)
	*/
	virtual int c2_create(c1s1* s1);
	
	/**
	* create s2.
	* random fill c2s2 1536 bytes
	* 
	* // server generate S2, or client valid S2
	* temp-key = HMACsha256(c1-digest, FMSKey, 68)
	* s2-digest-data = HMACsha256(s2-random-data, temp-key, 32)
	*/
	virtual int s2_create(c1s1* c1);
};

c2s2::c2s2()
{
	srs_random_generate(random, 1504);
	srs_random_generate(digest, 32);
}

c2s2::~c2s2()
{
}

void c2s2::dump(char* _c2s2)
{
	memcpy(_c2s2, random, 1504);
	memcpy(_c2s2 + 1504, digest, 32);
}

int c2s2::c2_create(c1s1* s1)
{
	int ret = ERROR_SUCCESS;
	
	char temp_key[OpensslHashSize];
	if ((ret = openssl_HMACsha256(s1->get_digest(), 32, SrsGenuineFPKey, 62, temp_key)) != ERROR_SUCCESS) {
		srs_error("create c2 temp key failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("generate c2 temp key success.");
	
	char _digest[OpensslHashSize];
	if ((ret = openssl_HMACsha256(random, 1504, temp_key, 32, _digest)) != ERROR_SUCCESS) {
		srs_error("create c2 digest failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("generate c2 digest success.");
	
	memcpy(digest, _digest, 32);
	
	return ret;
}

int c2s2::s2_create(c1s1* c1)
{
	int ret = ERROR_SUCCESS;
	
	char temp_key[OpensslHashSize];
	if ((ret = openssl_HMACsha256(c1->get_digest(), 32, SrsGenuineFMSKey, 68, temp_key)) != ERROR_SUCCESS) {
		srs_error("create s2 temp key failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("generate s2 temp key success.");
	
	char _digest[OpensslHashSize];
	if ((ret = openssl_HMACsha256(random, 1504, temp_key, 32, _digest)) != ERROR_SUCCESS) {
		srs_error("create s2 digest failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("generate s2 digest success.");
	
	memcpy(digest, _digest, 32);
	
	return ret;
}

c1s1::c1s1()
{
	schema = srs_schema_invalid;
}
c1s1::~c1s1()
{
	destroy_blocks();
}

char* c1s1::get_digest()
{
	srs_assert(schema != srs_schema_invalid);
	
	if (schema == srs_schema0) {
		return block1.digest.digest;
	} else {
		return block0.digest.digest;
	}
}

void c1s1::dump(char* _c1s1)
{
	srs_assert(schema != srs_schema_invalid);
	
	if (schema == srs_schema0) {
		srs_schema0_copy_to(_c1s1, true, time, version, &block0.key, &block1.digest);
	} else {
		srs_schema1_copy_to(_c1s1, true, time, version, &block0.digest, &block1.key);
	}
}

int c1s1::c1_create(srs_schema_type _schema)
{
	int ret = ERROR_SUCCESS;
	
	if (_schema == srs_schema_invalid) {
		ret = ERROR_RTMP_CH_SCHEMA;
		srs_error("create c1 failed. invalid schema=%d, ret=%d", _schema, ret);
		return ret;
	}
	
	destroy_blocks();
	
	time = ::time(NULL);
	version = 0x02070080; // client c1 version
	
	if (_schema == srs_schema0) {
		srs_key_block_init(&block0.key);
		srs_digest_block_init(&block1.digest);
	} else {
		srs_digest_block_init(&block0.digest);
		srs_key_block_init(&block1.key);
	}
	
	schema = _schema;
	
	char* digest = NULL;
	
	if ((ret = calc_c1_digest(digest)) != ERROR_SUCCESS) {
		srs_error("sign c1 error, failed to calc digest. ret=%d", ret);
		return ret;
	}
	
	srs_assert(digest != NULL);
	SrsAutoFree(char, digest, true);
	
	if (schema == srs_schema0) {
		memcpy(block1.digest.digest, digest, 32);
	} else {
		memcpy(block0.digest.digest, digest, 32);
	}
	
	return ret;
}

int c1s1::c1_parse(char* _c1s1, srs_schema_type _schema)
{
	int ret = ERROR_SUCCESS;
	
	if (_schema == srs_schema_invalid) {
		ret = ERROR_RTMP_CH_SCHEMA;
		srs_error("parse c1 failed. invalid schema=%d, ret=%d", _schema, ret);
		return ret;
	}
	
	destroy_blocks();
	
	time = *(int32_t*)_c1s1;
	version = *(int32_t*)(_c1s1 + 4); // client c1 version
	
	if (_schema == srs_schema0) {
		if ((ret = srs_key_block_parse(&block0.key, _c1s1 + 8)) != ERROR_SUCCESS) {
			srs_error("parse the c1 key failed. ret=%d", ret);
			return ret;
		}
		if ((ret = srs_digest_block_parse(&block1.digest, _c1s1 + 8 + 764)) != ERROR_SUCCESS) {
			srs_error("parse the c1 digest failed. ret=%d", ret);
			return ret;
		}
		srs_verbose("parse c1 key-digest success");
	} else if (_schema == srs_schema1) {
		if ((ret = srs_digest_block_parse(&block0.digest, _c1s1 + 8)) != ERROR_SUCCESS) {
			srs_error("parse the c1 key failed. ret=%d", ret);
			return ret;
		}
		if ((ret = srs_key_block_parse(&block1.key, _c1s1 + 8 + 764)) != ERROR_SUCCESS) {
			srs_error("parse the c1 digest failed. ret=%d", ret);
			return ret;
		}
		srs_verbose("parse c1 digest-key success");
	} else {
		ret = ERROR_RTMP_CH_SCHEMA;
		srs_error("parse c1 failed. invalid schema=%d, ret=%d", _schema, ret);
		return ret;
	}
	
	schema = _schema;
	
	return ret;
}

int c1s1::c1_validate_digest(bool& is_valid)
{
	int ret = ERROR_SUCCESS;
	
	char* c1_digest = NULL;
	
	if ((ret = calc_c1_digest(c1_digest)) != ERROR_SUCCESS) {
		srs_error("validate c1 error, failed to calc digest. ret=%d", ret);
		return ret;
	}
	
	srs_assert(c1_digest != NULL);
	SrsAutoFree(char, c1_digest, true);
	
	if (schema == srs_schema0) {
		is_valid = srs_bytes_equals(block1.digest.digest, c1_digest, 32);
	} else {
		is_valid = srs_bytes_equals(block0.digest.digest, c1_digest, 32);
	}
	
	return ret;
}

int c1s1::s1_create(c1s1* c1)
{
	int ret = ERROR_SUCCESS;
	
	if (c1->schema == srs_schema_invalid) {
		ret = ERROR_RTMP_CH_SCHEMA;
		srs_error("create s1 failed. invalid schema=%d, ret=%d", c1->schema, ret);
		return ret;
	}
	
	destroy_blocks();
	schema = c1->schema;
	
	time = ::time(NULL);
	version = 0x01000504; // server s1 version
	
	if (schema == srs_schema0) {
		srs_key_block_init(&block0.key);
		srs_digest_block_init(&block1.digest);
	} else {
		srs_digest_block_init(&block0.digest);
		srs_key_block_init(&block1.key);
	}
	
	if (schema == srs_schema0) {
		if ((ret = openssl_generate_key(c1->block0.key.key, block0.key.key, 128)) != ERROR_SUCCESS) {
			srs_error("calc s1 key failed. ret=%d", ret);
			return ret;
		}
	} else {
		if ((ret = openssl_generate_key(c1->block1.key.key, block1.key.key, 128)) != ERROR_SUCCESS) {
			srs_error("calc s1 key failed. ret=%d", ret);
			return ret;
		}
	}
	srs_verbose("calc s1 key success.");
		
	char* s1_digest = NULL;
	if ((ret = calc_s1_digest(s1_digest))  != ERROR_SUCCESS) {
		srs_error("calc s1 digest failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("calc s1 digest success.");
	
	srs_assert(s1_digest != NULL);
	SrsAutoFree(char, s1_digest, true);
	
	if (schema == srs_schema0) {
		memcpy(block1.digest.digest, s1_digest, 32);
	} else {
		memcpy(block0.digest.digest, s1_digest, 32);
	}
	srs_verbose("copy s1 key success.");
	
	return ret;
}

int c1s1::calc_s1_digest(char*& digest)
{
	int ret = ERROR_SUCCESS;
	
	srs_assert(schema == srs_schema0 || schema == srs_schema1);
	
	char* c1s1_joined_bytes = NULL;

	if (schema == srs_schema0) {
		c1s1_joined_bytes = srs_bytes_join_schema0(time, version, &block0.key, &block1.digest);
	} else {
		c1s1_joined_bytes = srs_bytes_join_schema1(time, version, &block0.digest, &block1.key);
	}
	
	srs_assert(c1s1_joined_bytes != NULL);
	SrsAutoFree(char, c1s1_joined_bytes, true);
	
	digest = new char[OpensslHashSize];
	if ((ret = openssl_HMACsha256(c1s1_joined_bytes, 1536 - 32, SrsGenuineFMSKey, 36, digest)) != ERROR_SUCCESS) {
		srs_error("calc digest for s1 failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("digest calculated for s1");
	
	return ret;
}

int c1s1::calc_c1_digest(char*& digest)
{
	int ret = ERROR_SUCCESS;
	
	srs_assert(schema == srs_schema0 || schema == srs_schema1);
	
	char* c1s1_joined_bytes = NULL;

	if (schema == srs_schema0) {
		c1s1_joined_bytes = srs_bytes_join_schema0(time, version, &block0.key, &block1.digest);
	} else {
		c1s1_joined_bytes = srs_bytes_join_schema1(time, version, &block0.digest, &block1.key);
	}
	
	srs_assert(c1s1_joined_bytes != NULL);
	SrsAutoFree(char, c1s1_joined_bytes, true);
	
	digest = new char[OpensslHashSize];
	if ((ret = openssl_HMACsha256(c1s1_joined_bytes, 1536 - 32, SrsGenuineFPKey, 30, digest)) != ERROR_SUCCESS) {
		srs_error("calc digest for c1 failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("digest calculated for c1");
	
	return ret;
}

void c1s1::destroy_blocks()
{
	if (schema == srs_schema_invalid) {
		return;
	}
	
	if (schema == srs_schema0) {
		srs_key_block_free(&block0.key);
		srs_digest_block_free(&block1.digest);
	} else {
		srs_digest_block_free(&block0.digest);
		srs_key_block_free(&block1.key);
	}
}

#endif

SrsSimpleHandshake::SrsSimpleHandshake()
{
}

SrsSimpleHandshake::~SrsSimpleHandshake()
{
}

int SrsSimpleHandshake::handshake_with_client(SrsSocket& skt, SrsComplexHandshake& complex_hs)
{
	int ret = ERROR_SUCCESS;
	
    ssize_t nsize;
    
    char* c0c1 = new char[1537];
    SrsAutoFree(char, c0c1, true);
    if ((ret = skt.read_fully(c0c1, 1537, &nsize)) != ERROR_SUCCESS) {
        srs_warn("read c0c1 failed. ret=%d", ret);
        return ret;
    }
    srs_verbose("read c0c1 success.");

	// plain text required.
	if (c0c1[0] != 0x03) {
		ret = ERROR_RTMP_PLAIN_REQUIRED;
		srs_warn("only support rtmp plain text. ret=%d", ret);
		return ret;
	}
    srs_verbose("check c0 success, required plain text.");
    
    // try complex handshake
    ret = complex_hs.handshake_with_client(skt, c0c1 + 1);
    if (ret == ERROR_SUCCESS) {
	    srs_trace("complex handshake success.");
	    return ret;
    }
    if (ret != ERROR_RTMP_TRY_SIMPLE_HS) {
	    srs_error("complex handshake failed. ret=%d", ret);
    	return ret;
    }
    srs_info("rollback complex to simple handshake. ret=%d", ret);
	
	char* s0s1s2 = new char[3073];
	srs_random_generate(s0s1s2, 3073);
    SrsAutoFree(char, s0s1s2, true);
	// plain text required.
    s0s1s2[0] = 0x03;
    if ((ret = skt.write(s0s1s2, 3073, &nsize)) != ERROR_SUCCESS) {
        srs_warn("simple handshake send s0s1s2 failed. ret=%d", ret);
        return ret;
    }
    srs_verbose("simple handshake send s0s1s2 success.");
    
    char* c2 = new char[1536];
    SrsAutoFree(char, c2, true);
    if ((ret = skt.read_fully(c2, 1536, &nsize)) != ERROR_SUCCESS) {
        srs_warn("simple handshake read c2 failed. ret=%d", ret);
        return ret;
    }
    srs_verbose("simple handshake read c2 success.");
    
    srs_trace("simple handshake success.");
    
	return ret;
}

int SrsSimpleHandshake::handshake_with_server(SrsSocket& skt, SrsComplexHandshake& complex_hs)
{
	int ret = ERROR_SUCCESS;
    
    // try complex handshake
    ret = complex_hs.handshake_with_server(skt);
    if (ret == ERROR_SUCCESS) {
	    srs_trace("complex handshake success.");
	    return ret;
    }
    if (ret != ERROR_RTMP_TRY_SIMPLE_HS) {
	    srs_error("complex handshake failed. ret=%d", ret);
    	return ret;
    }
    srs_info("rollback complex to simple handshake. ret=%d", ret);
	
	// simple handshake
    ssize_t nsize;
    
    char* c0c1 = new char[1537];
    SrsAutoFree(char, c0c1, true);
    
	srs_random_generate(c0c1, 1537);
	// plain text required.
	c0c1[0] = 0x03;
	
    if ((ret = skt.write(c0c1, 1537, &nsize)) != ERROR_SUCCESS) {
        srs_warn("write c0c1 failed. ret=%d", ret);
        return ret;
    }
    srs_verbose("write c0c1 success.");
	
	char* s0s1s2 = new char[3073];
    SrsAutoFree(char, s0s1s2, true);
    if ((ret = skt.read_fully(s0s1s2, 3073, &nsize)) != ERROR_SUCCESS) {
        srs_warn("simple handshake recv s0s1s2 failed. ret=%d", ret);
        return ret;
    }
    srs_verbose("simple handshake recv s0s1s2 success.");
    
	// plain text required.
    if (s0s1s2[0] != 0x03) {
        ret = ERROR_RTMP_HANDSHAKE;
        srs_warn("handshake failed, plain text required. ret=%d", ret);
        return ret;
    }
    
    char* c2 = new char[1536];
    SrsAutoFree(char, c2, true);
	srs_random_generate(c2, 1536);
    if ((ret = skt.write(c2, 1536, &nsize)) != ERROR_SUCCESS) {
        srs_warn("simple handshake write c2 failed. ret=%d", ret);
        return ret;
    }
    srs_verbose("simple handshake write c2 success.");
    
    srs_trace("simple handshake success.");
    
	return ret;
}

SrsComplexHandshake::SrsComplexHandshake()
{
}

SrsComplexHandshake::~SrsComplexHandshake()
{
}

#ifndef SRS_SSL
int SrsComplexHandshake::handshake_with_client(SrsSocket& /*skt*/, char* /*_c1*/)
{
	return ERROR_RTMP_TRY_SIMPLE_HS;
}
#else
int SrsComplexHandshake::handshake_with_client(SrsSocket& skt, char* _c1)
{
	int ret = ERROR_SUCCESS;

    ssize_t nsize;
	
	static bool _random_initialized = false;
	if (!_random_initialized) {
		srand(0);
		_random_initialized = true;
		srs_trace("srand initialized the random.");
	}
	
	// decode c1
	c1s1 c1;
	// try schema0.
	if ((ret = c1.c1_parse(_c1, srs_schema0)) != ERROR_SUCCESS) {
		srs_error("parse c1 schema%d error. ret=%d", srs_schema0, ret);
		return ret;
	}
	// try schema1
	bool is_valid = false;
	if ((ret = c1.c1_validate_digest(is_valid)) != ERROR_SUCCESS || !is_valid) {
		if ((ret = c1.c1_parse(_c1, srs_schema1)) != ERROR_SUCCESS) {
			srs_error("parse c1 schema%d error. ret=%d", srs_schema1, ret);
			return ret;
		}
		
		if ((ret = c1.c1_validate_digest(is_valid)) != ERROR_SUCCESS || !is_valid) {
			ret = ERROR_RTMP_TRY_SIMPLE_HS;
			srs_info("all schema valid failed, try simple handshake. ret=%d", ret);
			return ret;
		}
	}
	srs_verbose("decode c1 success.");
	
	// encode s1
	c1s1 s1;
	if ((ret = s1.s1_create(&c1)) != ERROR_SUCCESS) {
		srs_error("create s1 from c1 failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("create s1 from c1 success.");
	
	c2s2 s2;
	if ((ret = s2.s2_create(&c1)) != ERROR_SUCCESS) {
		srs_error("create s2 from c1 failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("create s2 from c1 success.");
	
	// sendout s0s1s2
	char* s0s1s2 = new char[3073];
    SrsAutoFree(char, s0s1s2, true);
	// plain text required.
	s0s1s2[0] = 0x03;
	s1.dump(s0s1s2 + 1);
	s2.dump(s0s1s2 + 1537);
    if ((ret = skt.write(s0s1s2, 3073, &nsize)) != ERROR_SUCCESS) {
        srs_warn("complex handshake send s0s1s2 failed. ret=%d", ret);
        return ret;
    }
    srs_verbose("complex handshake send s0s1s2 success.");
    
    // recv c2
    char* c2 = new char[1536];
    SrsAutoFree(char, c2, true);
    if ((ret = skt.read_fully(c2, 1536, &nsize)) != ERROR_SUCCESS) {
        srs_warn("complex handshake read c2 failed. ret=%d", ret);
        return ret;
    }
    srs_verbose("complex handshake read c2 success.");
	
	return ret;
}
#endif

#ifndef SRS_SSL
int SrsComplexHandshake::handshake_with_server(SrsSocket& /*skt*/)
{
	return ERROR_RTMP_TRY_SIMPLE_HS;
}
#else
int SrsComplexHandshake::handshake_with_server(SrsSocket& /*skt*/)
{
	int ret = ERROR_SUCCESS;
	
	// TODO: implements complex handshake.
	ret = ERROR_RTMP_TRY_SIMPLE_HS;
	
	return ret;
}
#endif

// AMF0 marker
#define RTMP_AMF0_Number 					0x00
#define RTMP_AMF0_Boolean 					0x01
#define RTMP_AMF0_String 					0x02
#define RTMP_AMF0_Object 					0x03
#define RTMP_AMF0_MovieClip 				0x04 // reserved, not supported
#define RTMP_AMF0_Null 						0x05
#define RTMP_AMF0_Undefined 				0x06
#define RTMP_AMF0_Reference 				0x07
#define RTMP_AMF0_EcmaArray 				0x08
#define RTMP_AMF0_ObjectEnd 				0x09
#define RTMP_AMF0_StrictArray 				0x0A
#define RTMP_AMF0_Date 						0x0B
#define RTMP_AMF0_LongString 				0x0C
#define RTMP_AMF0_UnSupported 				0x0D
#define RTMP_AMF0_RecordSet 				0x0E // reserved, not supported
#define RTMP_AMF0_XmlDocument 				0x0F
#define RTMP_AMF0_TypedObject 				0x10
// AVM+ object is the AMF3 object.
#define RTMP_AMF0_AVMplusObject 			0x11
// origin array whos data takes the same form as LengthValueBytes
#define RTMP_AMF0_OriginStrictArray 		0x20

// User defined
#define RTMP_AMF0_Invalid 					0x3F

int srs_amf0_get_object_eof_size();
int srs_amf0_get_any_size(SrsAmf0Any* value);
int srs_amf0_read_object_eof(SrsStream* stream, SrsAmf0ObjectEOF*&);
int srs_amf0_write_object_eof(SrsStream* stream, SrsAmf0ObjectEOF*);
int srs_amf0_read_any(SrsStream* stream, SrsAmf0Any*& value);
int srs_amf0_write_any(SrsStream* stream, SrsAmf0Any* value);

SrsAmf0Any::SrsAmf0Any()
{
	marker = RTMP_AMF0_Invalid;
}

SrsAmf0Any::~SrsAmf0Any()
{
}

bool SrsAmf0Any::is_string()
{
	return marker == RTMP_AMF0_String;
}

bool SrsAmf0Any::is_boolean()
{
	return marker == RTMP_AMF0_Boolean;
}

bool SrsAmf0Any::is_number()
{
	return marker == RTMP_AMF0_Number;
}

bool SrsAmf0Any::is_null()
{
	return marker == RTMP_AMF0_Null;
}

bool SrsAmf0Any::is_undefined()
{
	return marker == RTMP_AMF0_Undefined;
}

bool SrsAmf0Any::is_object()
{
	return marker == RTMP_AMF0_Object;
}

bool SrsAmf0Any::is_ecma_array()
{
	return marker == RTMP_AMF0_EcmaArray;
}

bool SrsAmf0Any::is_object_eof()
{
	return marker == RTMP_AMF0_ObjectEnd;
}

SrsAmf0String::SrsAmf0String(const char* _value)
{
	marker = RTMP_AMF0_String;
	if (_value) {
		value = _value;
	}
}

SrsAmf0String::~SrsAmf0String()
{
}

SrsAmf0Boolean::SrsAmf0Boolean(bool _value)
{
	marker = RTMP_AMF0_Boolean;
	value = _value;
}

SrsAmf0Boolean::~SrsAmf0Boolean()
{
}

SrsAmf0Number::SrsAmf0Number(double _value)
{
	marker = RTMP_AMF0_Number;
	value = _value;
}

SrsAmf0Number::~SrsAmf0Number()
{
}

SrsAmf0Null::SrsAmf0Null()
{
	marker = RTMP_AMF0_Null;
}

SrsAmf0Null::~SrsAmf0Null()
{
}

SrsAmf0Undefined::SrsAmf0Undefined()
{
	marker = RTMP_AMF0_Undefined;
}

SrsAmf0Undefined::~SrsAmf0Undefined()
{
}

SrsAmf0ObjectEOF::SrsAmf0ObjectEOF()
{
	marker = RTMP_AMF0_ObjectEnd;
	utf8_empty = 0x00;
}

SrsAmf0ObjectEOF::~SrsAmf0ObjectEOF()
{
}

SrsUnSortedHashtable::SrsUnSortedHashtable()
{
}

SrsUnSortedHashtable::~SrsUnSortedHashtable()
{
	std::vector<SrsObjectPropertyType>::iterator it;
	for (it = properties.begin(); it != properties.end(); ++it) {
		SrsObjectPropertyType& elem = *it;
		SrsAmf0Any* any = elem.second;
		srs_freep(any);
	}
	properties.clear();
}

int SrsUnSortedHashtable::size()
{
	return (int)properties.size();
}

void SrsUnSortedHashtable::clear()
{
	properties.clear();
}

std::string SrsUnSortedHashtable::key_at(int index)
{
	srs_assert(index < size());
	SrsObjectPropertyType& elem = properties[index];
	return elem.first;
}

SrsAmf0Any* SrsUnSortedHashtable::value_at(int index)
{
	srs_assert(index < size());
	SrsObjectPropertyType& elem = properties[index];
	return elem.second;
}

void SrsUnSortedHashtable::set(std::string key, SrsAmf0Any* value)
{
	std::vector<SrsObjectPropertyType>::iterator it;
	
	for (it = properties.begin(); it != properties.end(); ++it) {
		SrsObjectPropertyType& elem = *it;
		std::string name = elem.first;
		SrsAmf0Any* any = elem.second;
		
		if (key == name) {
			srs_freep(any);
			properties.erase(it);
			break;
		}
	}
	
	properties.push_back(std::make_pair(key, value));
}

SrsAmf0Any* SrsUnSortedHashtable::get_property(std::string name)
{
	std::vector<SrsObjectPropertyType>::iterator it;
	
	for (it = properties.begin(); it != properties.end(); ++it) {
		SrsObjectPropertyType& elem = *it;
		std::string key = elem.first;
		SrsAmf0Any* any = elem.second;
		if (key == name) {
			return any;
		}
	}
	
	return NULL;
}

SrsAmf0Any* SrsUnSortedHashtable::ensure_property_string(std::string name)
{
	SrsAmf0Any* prop = get_property(name);
	
	if (!prop) {
		return NULL;
	}
	
	if (!prop->is_string()) {
		return NULL;
	}
	
	return prop;
}

SrsAmf0Any* SrsUnSortedHashtable::ensure_property_number(std::string name)
{
	SrsAmf0Any* prop = get_property(name);
	
	if (!prop) {
		return NULL;
	}
	
	if (!prop->is_number()) {
		return NULL;
	}
	
	return prop;
}

SrsAmf0Object::SrsAmf0Object()
{
	marker = RTMP_AMF0_Object;
}

SrsAmf0Object::~SrsAmf0Object()
{
}

int SrsAmf0Object::size()
{
	return properties.size();
}

std::string SrsAmf0Object::key_at(int index)
{
	return properties.key_at(index);
}

SrsAmf0Any* SrsAmf0Object::value_at(int index)
{
	return properties.value_at(index);
}

void SrsAmf0Object::set(std::string key, SrsAmf0Any* value)
{
	properties.set(key, value);
}

SrsAmf0Any* SrsAmf0Object::get_property(std::string name)
{
	return properties.get_property(name);
}

SrsAmf0Any* SrsAmf0Object::ensure_property_string(std::string name)
{
	return properties.ensure_property_string(name);
}

SrsAmf0Any* SrsAmf0Object::ensure_property_number(std::string name)
{
	return properties.ensure_property_number(name);
}

SrsASrsAmf0EcmaArray::SrsASrsAmf0EcmaArray()
{
	marker = RTMP_AMF0_EcmaArray;
}

SrsASrsAmf0EcmaArray::~SrsASrsAmf0EcmaArray()
{
}

int SrsASrsAmf0EcmaArray::size()
{
	return properties.size();
}

void SrsASrsAmf0EcmaArray::clear()
{
	properties.clear();
}

std::string SrsASrsAmf0EcmaArray::key_at(int index)
{
	return properties.key_at(index);
}

SrsAmf0Any* SrsASrsAmf0EcmaArray::value_at(int index)
{
	return properties.value_at(index);
}

void SrsASrsAmf0EcmaArray::set(std::string key, SrsAmf0Any* value)
{
	properties.set(key, value);
}

SrsAmf0Any* SrsASrsAmf0EcmaArray::get_property(std::string name)
{
	return properties.get_property(name);
}

SrsAmf0Any* SrsASrsAmf0EcmaArray::ensure_property_string(std::string name)
{
	return properties.ensure_property_string(name);
}

int srs_amf0_read_utf8(SrsStream* stream, std::string& value)
{
	int ret = ERROR_SUCCESS;
	
	// len
	if (!stream->require(2)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read string length failed. ret=%d", ret);
		return ret;
	}
	int16_t len = stream->read_2bytes();
	srs_verbose("amf0 read string length success. len=%d", len);
	
	// empty string
	if (len <= 0) {
		srs_verbose("amf0 read empty string. ret=%d", ret);
		return ret;
	}
	
	// data
	if (!stream->require(len)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read string data failed. ret=%d", ret);
		return ret;
	}
	std::string str = stream->read_string(len);
	
	// support utf8-1 only
	// 1.3.1 Strings and UTF-8
	// UTF8-1 = %x00-7F
	for (int i = 0; i < len; i++) {
		char ch = *(str.data() + i);
		if ((ch & 0x80) != 0) {
			ret = ERROR_RTMP_AMF0_DECODE;
			srs_error("ignored. only support utf8-1, 0x00-0x7F, actual is %#x. ret=%d", (int)ch, ret);
			ret = ERROR_SUCCESS;
		}
	}
	
	value = str;
	srs_verbose("amf0 read string data success. str=%s", str.c_str());
	
	return ret;
}
int srs_amf0_write_utf8(SrsStream* stream, std::string value)
{
	int ret = ERROR_SUCCESS;
	
	// len
	if (!stream->require(2)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write string length failed. ret=%d", ret);
		return ret;
	}
	stream->write_2bytes(value.length());
	srs_verbose("amf0 write string length success. len=%d", (int)value.length());
	
	// empty string
	if (value.length() <= 0) {
		srs_verbose("amf0 write empty string. ret=%d", ret);
		return ret;
	}
	
	// data
	if (!stream->require(value.length())) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write string data failed. ret=%d", ret);
		return ret;
	}
	stream->write_string(value);
	srs_verbose("amf0 write string data success. str=%s", value.c_str());
	
	return ret;
}

int srs_amf0_read_string(SrsStream* stream, std::string& value)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read string marker failed. ret=%d", ret);
		return ret;
	}
	
	char marker = stream->read_1bytes();
	if (marker != RTMP_AMF0_String) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 check string marker failed. "
			"marker=%#x, required=%#x, ret=%d", marker, RTMP_AMF0_String, ret);
		return ret;
	}
	srs_verbose("amf0 read string marker success");
	
	return srs_amf0_read_utf8(stream, value);
}

int srs_amf0_write_string(SrsStream* stream, std::string value)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write string marker failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_1bytes(RTMP_AMF0_String);
	srs_verbose("amf0 write string marker success");
	
	return srs_amf0_write_utf8(stream, value);
}

int srs_amf0_read_boolean(SrsStream* stream, bool& value)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read bool marker failed. ret=%d", ret);
		return ret;
	}
	
	char marker = stream->read_1bytes();
	if (marker != RTMP_AMF0_Boolean) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 check bool marker failed. "
			"marker=%#x, required=%#x, ret=%d", marker, RTMP_AMF0_Boolean, ret);
		return ret;
	}
	srs_verbose("amf0 read bool marker success");

	// value
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read bool value failed. ret=%d", ret);
		return ret;
	}

	if (stream->read_1bytes() == 0) {
		value = false;
	} else {
		value = true;
	}
	
	srs_verbose("amf0 read bool value success. value=%d", value);
	
	return ret;
}
int srs_amf0_write_boolean(SrsStream* stream, bool value)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write bool marker failed. ret=%d", ret);
		return ret;
	}
	stream->write_1bytes(RTMP_AMF0_Boolean);
	srs_verbose("amf0 write bool marker success");

	// value
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write bool value failed. ret=%d", ret);
		return ret;
	}

	if (value) {
		stream->write_1bytes(0x01);
	} else {
		stream->write_1bytes(0x00);
	}
	
	srs_verbose("amf0 write bool value success. value=%d", value);
	
	return ret;
}

int srs_amf0_read_number(SrsStream* stream, double& value)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read number marker failed. ret=%d", ret);
		return ret;
	}
	
	char marker = stream->read_1bytes();
	if (marker != RTMP_AMF0_Number) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 check number marker failed. "
			"marker=%#x, required=%#x, ret=%d", marker, RTMP_AMF0_Number, ret);
		return ret;
	}
	srs_verbose("amf0 read number marker success");

	// value
	if (!stream->require(8)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read number value failed. ret=%d", ret);
		return ret;
	}

	int64_t temp = stream->read_8bytes();
	memcpy(&value, &temp, 8);
	
	srs_verbose("amf0 read number value success. value=%.2f", value);
	
	return ret;
}
int srs_amf0_write_number(SrsStream* stream, double value)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write number marker failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_1bytes(RTMP_AMF0_Number);
	srs_verbose("amf0 write number marker success");

	// value
	if (!stream->require(8)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write number value failed. ret=%d", ret);
		return ret;
	}

	int64_t temp = 0x00;
	memcpy(&temp, &value, 8);
	stream->write_8bytes(temp);
	
	srs_verbose("amf0 write number value success. value=%.2f", value);
	
	return ret;
}

int srs_amf0_read_null(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read null marker failed. ret=%d", ret);
		return ret;
	}
	
	char marker = stream->read_1bytes();
	if (marker != RTMP_AMF0_Null) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 check null marker failed. "
			"marker=%#x, required=%#x, ret=%d", marker, RTMP_AMF0_Null, ret);
		return ret;
	}
	srs_verbose("amf0 read null success");
	
	return ret;
}
int srs_amf0_write_null(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write null marker failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_1bytes(RTMP_AMF0_Null);
	srs_verbose("amf0 write null marker success");
	
	return ret;
}

int srs_amf0_read_undefined(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read undefined marker failed. ret=%d", ret);
		return ret;
	}
	
	char marker = stream->read_1bytes();
	if (marker != RTMP_AMF0_Undefined) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 check undefined marker failed. "
			"marker=%#x, required=%#x, ret=%d", marker, RTMP_AMF0_Undefined, ret);
		return ret;
	}
	srs_verbose("amf0 read undefined success");
	
	return ret;
}
int srs_amf0_write_undefined(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write undefined marker failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_1bytes(RTMP_AMF0_Undefined);
	srs_verbose("amf0 write undefined marker success");
	
	return ret;
}

int srs_amf0_read_any(SrsStream* stream, SrsAmf0Any*& value)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read any marker failed. ret=%d", ret);
		return ret;
	}
	
	char marker = stream->read_1bytes();
	srs_verbose("amf0 any marker success");
	
	// backward the 1byte marker.
	stream->skip(-1);
	
	switch (marker) {
		case RTMP_AMF0_String: {
			std::string data;
			if ((ret = srs_amf0_read_string(stream, data)) != ERROR_SUCCESS) {
				return ret;
			}
			value = new SrsAmf0String();
			srs_amf0_convert<SrsAmf0String>(value)->value = data;
			return ret;
		}
		case RTMP_AMF0_Boolean: {
			bool data;
			if ((ret = srs_amf0_read_boolean(stream, data)) != ERROR_SUCCESS) {
				return ret;
			}
			value = new SrsAmf0Boolean();
			srs_amf0_convert<SrsAmf0Boolean>(value)->value = data;
			return ret;
		}
		case RTMP_AMF0_Number: {
			double data;
			if ((ret = srs_amf0_read_number(stream, data)) != ERROR_SUCCESS) {
				return ret;
			}
			value = new SrsAmf0Number();
			srs_amf0_convert<SrsAmf0Number>(value)->value = data;
			return ret;
		}
		case RTMP_AMF0_Null: {
			stream->skip(1);
			value = new SrsAmf0Null();
			return ret;
		}
		case RTMP_AMF0_Undefined: {
			stream->skip(1);
			value = new SrsAmf0Undefined();
			return ret;
		}
		case RTMP_AMF0_ObjectEnd: {
			SrsAmf0ObjectEOF* p = NULL;
			if ((ret = srs_amf0_read_object_eof(stream, p)) != ERROR_SUCCESS) {
				return ret;
			}
			value = p;
			return ret;
		}
		case RTMP_AMF0_Object: {
			SrsAmf0Object* p = NULL;
			if ((ret = srs_amf0_read_object(stream, p)) != ERROR_SUCCESS) {
				return ret;
			}
			value = p;
			return ret;
		}
		case RTMP_AMF0_EcmaArray: {
			SrsASrsAmf0EcmaArray* p = NULL;
			if ((ret = srs_amf0_read_ecma_array(stream, p)) != ERROR_SUCCESS) {
				return ret;
			}
			value = p;
			return ret;
		}
		case RTMP_AMF0_Invalid:
		default: {
			ret = ERROR_RTMP_AMF0_INVALID;
			srs_error("invalid amf0 message type. marker=%#x, ret=%d", marker, ret);
			return ret;
		}
	}
	
	return ret;
}
int srs_amf0_write_any(SrsStream* stream, SrsAmf0Any* value)
{
	int ret = ERROR_SUCCESS;
	
	srs_assert(value != NULL);
	
	switch (value->marker) {
		case RTMP_AMF0_String: {
			std::string data = srs_amf0_convert<SrsAmf0String>(value)->value;
			return srs_amf0_write_string(stream, data);
		}
		case RTMP_AMF0_Boolean: {
			bool data = srs_amf0_convert<SrsAmf0Boolean>(value)->value;
			return srs_amf0_write_boolean(stream, data);
		}
		case RTMP_AMF0_Number: {
			double data = srs_amf0_convert<SrsAmf0Number>(value)->value;
			return srs_amf0_write_number(stream, data);
		}
		case RTMP_AMF0_Null: {
			return srs_amf0_write_null(stream);
		}
		case RTMP_AMF0_Undefined: {
			return srs_amf0_write_undefined(stream);
		}
		case RTMP_AMF0_ObjectEnd: {
			SrsAmf0ObjectEOF* p = srs_amf0_convert<SrsAmf0ObjectEOF>(value);
			return srs_amf0_write_object_eof(stream, p);
		}
		case RTMP_AMF0_Object: {
			SrsAmf0Object* p = srs_amf0_convert<SrsAmf0Object>(value);
			return srs_amf0_write_object(stream, p);
		}
		case RTMP_AMF0_EcmaArray: {
			SrsASrsAmf0EcmaArray* p = srs_amf0_convert<SrsASrsAmf0EcmaArray>(value);
			return srs_amf0_write_ecma_array(stream, p);
		}
		case RTMP_AMF0_Invalid:
		default: {
			ret = ERROR_RTMP_AMF0_INVALID;
			srs_error("invalid amf0 message type. marker=%#x, ret=%d", value->marker, ret);
			return ret;
		}
	}
	
	return ret;
}
int srs_amf0_get_any_size(SrsAmf0Any* value)
{
	if (!value) {
		return 0;
	}
	
	int size = 0;
	
	switch (value->marker) {
		case RTMP_AMF0_String: {
			SrsAmf0String* p = srs_amf0_convert<SrsAmf0String>(value);
			size += srs_amf0_get_string_size(p->value);
			break;
		}
		case RTMP_AMF0_Boolean: {
			size += srs_amf0_get_boolean_size();
			break;
		}
		case RTMP_AMF0_Number: {
			size += srs_amf0_get_number_size();
			break;
		}
		case RTMP_AMF0_Null: {
			size += srs_amf0_get_null_size();
			break;
		}
		case RTMP_AMF0_Undefined: {
			size += srs_amf0_get_undefined_size();
			break;
		}
		case RTMP_AMF0_ObjectEnd: {
			size += srs_amf0_get_object_eof_size();
			break;
		}
		case RTMP_AMF0_Object: {
			SrsAmf0Object* p = srs_amf0_convert<SrsAmf0Object>(value);
			size += srs_amf0_get_object_size(p);
			break;
		}
		case RTMP_AMF0_EcmaArray: {
			SrsASrsAmf0EcmaArray* p = srs_amf0_convert<SrsASrsAmf0EcmaArray>(value);
			size += srs_amf0_get_ecma_array_size(p);
			break;
		}
		default: {
			// TOOD: other AMF0 types.
			srs_warn("ignore unkown AMF0 type size.");
			break;
		}
	}
	
	return size;
}

int srs_amf0_read_object_eof(SrsStream* stream, SrsAmf0ObjectEOF*& value)
{
	int ret = ERROR_SUCCESS;
	
	// auto skip -2 to read the object eof.
	srs_assert(stream->pos() >= 2);
	stream->skip(-2);
	
	// value
	if (!stream->require(2)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read object eof value failed. ret=%d", ret);
		return ret;
	}
	int16_t temp = stream->read_2bytes();
	if (temp != 0x00) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read object eof value check failed. "
			"must be 0x00, actual is %#x, ret=%d", temp, ret);
		return ret;
	}
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read object eof marker failed. ret=%d", ret);
		return ret;
	}
	
	char marker = stream->read_1bytes();
	if (marker != RTMP_AMF0_ObjectEnd) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 check object eof marker failed. "
			"marker=%#x, required=%#x, ret=%d", marker, RTMP_AMF0_ObjectEnd, ret);
		return ret;
	}
	srs_verbose("amf0 read object eof marker success");
	
	value = new SrsAmf0ObjectEOF();
	srs_verbose("amf0 read object eof success");
	
	return ret;
}
int srs_amf0_write_object_eof(SrsStream* stream, SrsAmf0ObjectEOF* value)
{
	int ret = ERROR_SUCCESS;
	
	srs_assert(value != NULL);
	
	// value
	if (!stream->require(2)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write object eof value failed. ret=%d", ret);
		return ret;
	}
	stream->write_2bytes(0x00);
	srs_verbose("amf0 write object eof value success");
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write object eof marker failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_1bytes(RTMP_AMF0_ObjectEnd);
	
	srs_verbose("amf0 read object eof success");
	
	return ret;
}

int srs_amf0_read_object(SrsStream* stream, SrsAmf0Object*& value)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read object marker failed. ret=%d", ret);
		return ret;
	}
	
	char marker = stream->read_1bytes();
	if (marker != RTMP_AMF0_Object) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 check object marker failed. "
			"marker=%#x, required=%#x, ret=%d", marker, RTMP_AMF0_Object, ret);
		return ret;
	}
	srs_verbose("amf0 read object marker success");
	
	// value
	value = new SrsAmf0Object();

	while (!stream->empty()) {
		// property-name: utf8 string
		std::string property_name;
		if ((ret =srs_amf0_read_utf8(stream, property_name)) != ERROR_SUCCESS) {
			srs_error("amf0 object read property name failed. ret=%d", ret);
			return ret;
		}
		// property-value: any
		SrsAmf0Any* property_value = NULL;
		if ((ret = srs_amf0_read_any(stream, property_value)) != ERROR_SUCCESS) {
			srs_error("amf0 object read property_value failed. "
				"name=%s, ret=%d", property_name.c_str(), ret);
			return ret;
		}
		
		// AMF0 Object EOF.
		if (property_name.empty() || !property_value || property_value->is_object_eof()) {
			if (property_value) {
				srs_freep(property_value);
			}
			srs_info("amf0 read object EOF.");
			break;
		}
		
		// add property
		value->set(property_name, property_value);
	}
	
	return ret;
}
int srs_amf0_write_object(SrsStream* stream, SrsAmf0Object* value)
{
	int ret = ERROR_SUCCESS;

	srs_assert(value != NULL);
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write object marker failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_1bytes(RTMP_AMF0_Object);
	srs_verbose("amf0 write object marker success");
	
	// value
	for (int i = 0; i < value->size(); i++) {
		std::string name = value->key_at(i);
		SrsAmf0Any* any = value->value_at(i);
		
		if ((ret = srs_amf0_write_utf8(stream, name)) != ERROR_SUCCESS) {
			srs_error("write object property name failed. ret=%d", ret);
			return ret;
		}
		
		if ((ret = srs_amf0_write_any(stream, any)) != ERROR_SUCCESS) {
			srs_error("write object property value failed. ret=%d", ret);
			return ret;
		}
		
		srs_verbose("write amf0 property success. name=%s", name.c_str());
	}
	
	if ((ret = srs_amf0_write_object_eof(stream, &value->eof)) != ERROR_SUCCESS) {
		srs_error("write object eof failed. ret=%d", ret);
		return ret;
	}
	
	srs_verbose("write amf0 object success.");
	
	return ret;
}

int srs_amf0_read_ecma_array(SrsStream* stream, SrsASrsAmf0EcmaArray*& value)
{
	int ret = ERROR_SUCCESS;
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read ecma_array marker failed. ret=%d", ret);
		return ret;
	}
	
	char marker = stream->read_1bytes();
	if (marker != RTMP_AMF0_EcmaArray) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 check ecma_array marker failed. "
			"marker=%#x, required=%#x, ret=%d", marker, RTMP_AMF0_Object, ret);
		return ret;
	}
	srs_verbose("amf0 read ecma_array marker success");

	// count
	if (!stream->require(4)) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 read ecma_array count failed. ret=%d", ret);
		return ret;
	}
	
	int32_t count = stream->read_4bytes();
	srs_verbose("amf0 read ecma_array count success. count=%d", count);
	
	// value
	value = new SrsASrsAmf0EcmaArray();
	value->count = count;

	while (!stream->empty()) {
		// property-name: utf8 string
		std::string property_name;
		if ((ret =srs_amf0_read_utf8(stream, property_name)) != ERROR_SUCCESS) {
			srs_error("amf0 ecma_array read property name failed. ret=%d", ret);
			return ret;
		}
		// property-value: any
		SrsAmf0Any* property_value = NULL;
		if ((ret = srs_amf0_read_any(stream, property_value)) != ERROR_SUCCESS) {
			srs_error("amf0 ecma_array read property_value failed. "
				"name=%s, ret=%d", property_name.c_str(), ret);
			return ret;
		}
		
		// AMF0 Object EOF.
		if (property_name.empty() || !property_value || property_value->is_object_eof()) {
			if (property_value) {
				srs_freep(property_value);
			}
			srs_info("amf0 read ecma_array EOF.");
			break;
		}
		
		// add property
		value->set(property_name, property_value);
	}
	
	return ret;
}
int srs_amf0_write_ecma_array(SrsStream* stream, SrsASrsAmf0EcmaArray* value)
{
	int ret = ERROR_SUCCESS;

	srs_assert(value != NULL);
	
	// marker
	if (!stream->require(1)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write ecma_array marker failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_1bytes(RTMP_AMF0_EcmaArray);
	srs_verbose("amf0 write ecma_array marker success");

	// count
	if (!stream->require(4)) {
		ret = ERROR_RTMP_AMF0_ENCODE;
		srs_error("amf0 write ecma_array count failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_4bytes(value->count);
	srs_verbose("amf0 write ecma_array count success. count=%d", value->count);
	
	// value
	for (int i = 0; i < value->size(); i++) {
		std::string name = value->key_at(i);
		SrsAmf0Any* any = value->value_at(i);
		
		if ((ret = srs_amf0_write_utf8(stream, name)) != ERROR_SUCCESS) {
			srs_error("write ecma_array property name failed. ret=%d", ret);
			return ret;
		}
		
		if ((ret = srs_amf0_write_any(stream, any)) != ERROR_SUCCESS) {
			srs_error("write ecma_array property value failed. ret=%d", ret);
			return ret;
		}
		
		srs_verbose("write amf0 property success. name=%s", name.c_str());
	}
	
	if ((ret = srs_amf0_write_object_eof(stream, &value->eof)) != ERROR_SUCCESS) {
		srs_error("write ecma_array eof failed. ret=%d", ret);
		return ret;
	}
	
	srs_verbose("write ecma_array object success.");
	
	return ret;
}

int srs_amf0_get_utf8_size(std::string value)
{
	return 2 + value.length();
}

int srs_amf0_get_string_size(std::string value)
{
	return 1 + srs_amf0_get_utf8_size(value);
}

int srs_amf0_get_number_size()
{
	return 1 + 8;
}

int srs_amf0_get_null_size()
{
	return 1;
}

int srs_amf0_get_undefined_size()
{
	return 1;
}

int srs_amf0_get_boolean_size()
{
	return 1 + 1;
}

int srs_amf0_get_object_size(SrsAmf0Object* obj)
{
	if (!obj) {
		return 0;
	}
	
	int size = 1;
	
	for (int i = 0; i < obj->size(); i++){
		std::string name = obj->key_at(i);
		SrsAmf0Any* value = obj->value_at(i);
		
		size += srs_amf0_get_utf8_size(name);
		size += srs_amf0_get_any_size(value);
	}
	
	size += srs_amf0_get_object_eof_size();
	
	return size;
}

int srs_amf0_get_ecma_array_size(SrsASrsAmf0EcmaArray* arr)
{
	if (!arr) {
		return 0;
	}
	
	int size = 1 + 4;
	
	for (int i = 0; i < arr->size(); i++){
		std::string name = arr->key_at(i);
		SrsAmf0Any* value = arr->value_at(i);
		
		size += srs_amf0_get_utf8_size(name);
		size += srs_amf0_get_any_size(value);
	}
	
	size += srs_amf0_get_object_eof_size();
	
	return size;
}

int srs_amf0_get_object_eof_size()
{
	return 2 + 1;
}

/**
* the signature for packets to client.
*/
#define RTMP_SIG_FMS_VER 					"3,5,3,888"
#define RTMP_SIG_AMF0_VER 					0
#define RTMP_SIG_CLIENT_ID 					"ASAICiss"

/**
* onStatus consts.
*/
#define StatusLevel 						"level"
#define StatusCode 							"code"
#define StatusDescription 					"description"
#define StatusDetails 						"details"
#define StatusClientId 						"clientid"
// status value
#define StatusLevelStatus 					"status"
// code value
#define StatusCodeConnectSuccess 			"NetConnection.Connect.Success"
#define StatusCodeStreamReset 				"NetStream.Play.Reset"
#define StatusCodeStreamStart 				"NetStream.Play.Start"
#define StatusCodeStreamPause 				"NetStream.Pause.Notify"
#define StatusCodeStreamUnpause 			"NetStream.Unpause.Notify"
#define StatusCodePublishStart 				"NetStream.Publish.Start"
#define StatusCodeDataStart 				"NetStream.Data.Start"
#define StatusCodeUnpublishSuccess 			"NetStream.Unpublish.Success"

// FMLE
#define RTMP_AMF0_COMMAND_ON_FC_PUBLISH		"onFCPublish"
#define RTMP_AMF0_COMMAND_ON_FC_UNPUBLISH	"onFCUnpublish"

// default stream id for response the createStream request.
#define SRS_DEFAULT_SID 					1

SrsRequest::SrsRequest()
{
	objectEncoding = RTMP_SIG_AMF0_VER;
}

SrsRequest::~SrsRequest()
{
}

#if 0 //disable the server-side behavior.
int SrsRequest::discovery_app()
{
	int ret = ERROR_SUCCESS;
	
	size_t pos = std::string::npos;
	std::string url = tcUrl;
	
	if ((pos = url.find("://")) != std::string::npos) {
		schema = url.substr(0, pos);
		url = url.substr(schema.length() + 3);
		srs_verbose("discovery schema=%s", schema.c_str());
	}
	
	if ((pos = url.find("/")) != std::string::npos) {
		vhost = url.substr(0, pos);
		url = url.substr(vhost.length() + 1);
		srs_verbose("discovery vhost=%s", vhost.c_str());
	}

	port = RTMP_DEFAULT_PORTS;
	if ((pos = vhost.find(":")) != std::string::npos) {
		port = vhost.substr(pos + 1);
		vhost = vhost.substr(0, pos);
		srs_verbose("discovery vhost=%s, port=%s", vhost.c_str(), port.c_str());
	}
	
	app = url;
	srs_vhost_resolve(vhost, app);
	
	// resolve the vhost from config
	SrsConfDirective* parsed_vhost = config->get_vhost(vhost);
	if (parsed_vhost) {
		vhost = parsed_vhost->arg0();
	}
	
	srs_info("discovery app success. schema=%s, vhost=%s, port=%s, app=%s",
		schema.c_str(), vhost.c_str(), port.c_str(), app.c_str());
	
	if (schema.empty() || vhost.empty() || port.empty() || app.empty()) {
		ret = ERROR_RTMP_REQ_TCURL;
		srs_error("discovery tcUrl failed. "
			"tcUrl=%s, schema=%s, vhost=%s, port=%s, app=%s, ret=%d",
			tcUrl.c_str(), schema.c_str(), vhost.c_str(), port.c_str(), app.c_str(), ret);
		return ret;
	}
	
	strip();
	
	return ret;
}
#endif

std::string SrsRequest::get_stream_url()
{
	std::string url = "";
	
	url += vhost;
	url += "/";
	url += app;
	url += "/";
	url += stream;

	return url;
}

void SrsRequest::strip()
{
	trim(vhost, "/ \n\r\t");
	trim(app, "/ \n\r\t");
	trim(stream, "/ \n\r\t");
}

std::string& SrsRequest::trim(std::string& str, std::string chs)
{
	for (int i = 0; i < (int)chs.length(); i++) {
		char ch = chs.at(i);
		
		for (std::string::iterator it = str.begin(); it != str.end();) {
			if (ch == *it) {
				it = str.erase(it);
			} else {
				++it;
			}
		}
	}
	
	return str;
}

SrsResponse::SrsResponse()
{
	stream_id = SRS_DEFAULT_SID;
}

SrsResponse::~SrsResponse()
{
}

/****************************************************************************
*****************************************************************************
****************************************************************************/
/**
5. Protocol Control Messages
RTMP reserves message type IDs 1-7 for protocol control messages.
These messages contain information needed by the RTM Chunk Stream
protocol or RTMP itself. Protocol messages with IDs 1 & 2 are
reserved for usage with RTM Chunk Stream protocol. Protocol messages
with IDs 3-6 are reserved for usage of RTMP. Protocol message with ID
7 is used between edge server and origin server.
*/
#define RTMP_MSG_SetChunkSize 				0x01
#define RTMP_MSG_AbortMessage 				0x02
#define RTMP_MSG_Acknowledgement 			0x03
#define RTMP_MSG_UserControlMessage 		0x04
#define RTMP_MSG_WindowAcknowledgementSize 	0x05
#define RTMP_MSG_SetPeerBandwidth 			0x06
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
#define RTMP_MSG_AMF3CommandMessage 		17 // 0x11
#define RTMP_MSG_AMF0CommandMessage 		20 // 0x14
/**
3.2. Data message
The client or the server sends this message to send Metadata or any
user data to the peer. Metadata includes details about the
data(audio, video etc.) like creation time, duration, theme and so
on. These messages have been assigned message type value of 18 for
AMF0 and message type value of 15 for AMF3.        
*/
#define RTMP_MSG_AMF0DataMessage 			18 // 0x12
#define RTMP_MSG_AMF3DataMessage 			15 // 0x0F
/**
3.3. Shared object message
A shared object is a Flash object (a collection of name value pairs)
that are in synchronization across multiple clients, instances, and
so on. The message types kMsgContainer=19 for AMF0 and
kMsgContainerEx=16 for AMF3 are reserved for shared object events.
Each message can contain multiple events.
*/
#define RTMP_MSG_AMF3SharedObject 			16 // 0x10
#define RTMP_MSG_AMF0SharedObject 			19 // 0x13
/**
3.4. Audio message
The client or the server sends this message to send audio data to the
peer. The message type value of 8 is reserved for audio messages.
*/
#define RTMP_MSG_AudioMessage 				8 // 0x08
/* *
3.5. Video message
The client or the server sends this message to send video data to the
peer. The message type value of 9 is reserved for video messages.
These messages are large and can delay the sending of other type of
messages. To avoid such a situation, the video message is assigned
the lowest priority.
*/
#define RTMP_MSG_VideoMessage 				9 // 0x09
/**
3.6. Aggregate message
An aggregate message is a single message that contains a list of submessages.
The message type value of 22 is reserved for aggregate
messages.
*/
#define RTMP_MSG_AggregateMessage 			22 // 0x16

/****************************************************************************
*****************************************************************************
****************************************************************************/
/**
* 6.1.2. Chunk Message Header
* There are four different formats for the chunk message header,
* selected by the "fmt" field in the chunk basic header.
*/
// 6.1.2.1. Type 0
// Chunks of Type 0 are 11 bytes long. This type MUST be used at the
// start of a chunk stream, and whenever the stream timestamp goes
// backward (e.g., because of a backward seek).
#define RTMP_FMT_TYPE0 						0
// 6.1.2.2. Type 1
// Chunks of Type 1 are 7 bytes long. The message stream ID is not
// included; this chunk takes the same stream ID as the preceding chunk.
// Streams with variable-sized messages (for example, many video
// formats) SHOULD use this format for the first chunk of each new
// message after the first.
#define RTMP_FMT_TYPE1 						1
// 6.1.2.3. Type 2
// Chunks of Type 2 are 3 bytes long. Neither the stream ID nor the
// message length is included; this chunk has the same stream ID and
// message length as the preceding chunk. Streams with constant-sized
// messages (for example, some audio and data formats) SHOULD use this
// format for the first chunk of each message after the first.
#define RTMP_FMT_TYPE2 						2
// 6.1.2.4. Type 3
// Chunks of Type 3 have no header. Stream ID, message length and
// timestamp delta are not present; chunks of this type take values from
// the preceding chunk. When a single message is split into chunks, all
// chunks of a message except the first one, SHOULD use this type. Refer
// to example 2 in section 6.2.2. Stream consisting of messages of
// exactly the same size, stream ID and spacing in time SHOULD use this
// type for all chunks after chunk of Type 2. Refer to example 1 in
// section 6.2.1. If the delta between the first message and the second
// message is same as the time stamp of first message, then chunk of
// type 3 would immediately follow the chunk of type 0 as there is no
// need for a chunk of type 2 to register the delta. If Type 3 chunk
// follows a Type 0 chunk, then timestamp delta for this Type 3 chunk is
// the same as the timestamp of Type 0 chunk.
#define RTMP_FMT_TYPE3 						3

/****************************************************************************
*****************************************************************************
****************************************************************************/
/**
* 6. Chunking
* The chunk size is configurable. It can be set using a control
* message(Set Chunk Size) as described in section 7.1. The maximum
* chunk size can be 65536 bytes and minimum 128 bytes. Larger values
* reduce CPU usage, but also commit to larger writes that can delay
* other content on lower bandwidth connections. Smaller chunks are not
* good for high-bit rate streaming. Chunk size is maintained
* independently for each direction.
*/
#define RTMP_DEFAULT_CHUNK_SIZE 			128
#define RTMP_MIN_CHUNK_SIZE 				2

/**
* 6.1. Chunk Format
* Extended timestamp: 0 or 4 bytes
* This field MUST be sent when the normal timsestamp is set to
* 0xffffff, it MUST NOT be sent if the normal timestamp is set to
* anything else. So for values less than 0xffffff the normal
* timestamp field SHOULD be used in which case the extended timestamp
* MUST NOT be present. For values greater than or equal to 0xffffff
* the normal timestamp field MUST NOT be used and MUST be set to
* 0xffffff and the extended timestamp MUST be sent.
*/
#define RTMP_EXTENDED_TIMESTAMP 			0xFFFFFF

/****************************************************************************
*****************************************************************************
****************************************************************************/
/**
* amf0 command message, command name macros
*/
#define RTMP_AMF0_COMMAND_CONNECT			"connect"
#define RTMP_AMF0_COMMAND_CREATE_STREAM		"createStream"
#define RTMP_AMF0_COMMAND_PLAY				"play"
#define RTMP_AMF0_COMMAND_PAUSE				"pause"
#define RTMP_AMF0_COMMAND_ON_BW_DONE		"onBWDone"
#define RTMP_AMF0_COMMAND_ON_STATUS			"onStatus"
#define RTMP_AMF0_COMMAND_RESULT			"_result"
#define RTMP_AMF0_COMMAND_ERROR				"_error"
#define RTMP_AMF0_COMMAND_RELEASE_STREAM	"releaseStream"
#define RTMP_AMF0_COMMAND_FC_PUBLISH		"FCPublish"
#define RTMP_AMF0_COMMAND_UNPUBLISH			"FCUnpublish"
#define RTMP_AMF0_COMMAND_PUBLISH			"publish"
#define RTMP_AMF0_DATA_SAMPLE_ACCESS		"|RtmpSampleAccess"
#define RTMP_AMF0_DATA_SET_DATAFRAME		"@setDataFrame"
#define RTMP_AMF0_DATA_ON_METADATA			"onMetaData"

/****************************************************************************
*****************************************************************************
****************************************************************************/
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

/****************************************************************************
*****************************************************************************
****************************************************************************/
// when got a messae header, increase recv timeout to got an entire message.
#define SRS_MIN_RECV_TIMEOUT_US 3000

SrsProtocol::AckWindowSize::AckWindowSize()
{
	ack_window_size = acked_size = 0;
}

SrsProtocol::SrsProtocol(st_netfd_t client_stfd)
{
	stfd = client_stfd;
	buffer = new SrsBuffer();
	skt = new SrsSocket(stfd);
	
	in_chunk_size = out_chunk_size = RTMP_DEFAULT_CHUNK_SIZE;
}

SrsProtocol::~SrsProtocol()
{
	std::map<int, SrsChunkStream*>::iterator it;
	
	for (it = chunk_streams.begin(); it != chunk_streams.end(); ++it) {
		SrsChunkStream* stream = it->second;
		srs_freep(stream);
	}

	chunk_streams.clear();
	
	srs_freep(buffer);
	srs_freep(skt);
}

std::string SrsProtocol::get_request_name(double transcationId)
{
	if (requests.find(transcationId) == requests.end()) {
		return "";
	}
	
	return requests[transcationId];
}

void SrsProtocol::set_recv_timeout(int64_t timeout_us)
{
	return skt->set_recv_timeout(timeout_us);
}

int64_t SrsProtocol::get_recv_timeout()
{
	return skt->get_recv_timeout();
}

void SrsProtocol::set_send_timeout(int64_t timeout_us)
{
	return skt->set_send_timeout(timeout_us);
}

int64_t SrsProtocol::get_recv_bytes()
{
	return skt->get_recv_bytes();
}

int64_t SrsProtocol::get_send_bytes()
{
	return skt->get_send_bytes();
}

int SrsProtocol::get_recv_kbps()
{
	return skt->get_recv_kbps();
}

int SrsProtocol::get_send_kbps()
{
	return skt->get_send_kbps();
}

int SrsProtocol::recv_message(SrsCommonMessage** pmsg)
{
	*pmsg = NULL;
	
	int ret = ERROR_SUCCESS;
	
	while (true) {
		SrsCommonMessage* msg = NULL;
		
		if ((ret = recv_interlaced_message(&msg)) != ERROR_SUCCESS) {
			if (ret != ERROR_SOCKET_TIMEOUT) {
				srs_error("recv interlaced message failed. ret=%d", ret);
			}
			return ret;
		}
		srs_verbose("entire msg received");
		
		if (!msg) {
			continue;
		}
		
		if (msg->size <= 0 || msg->header.payload_length <= 0) {
			srs_trace("ignore empty message(type=%d, size=%d, time=%d, sid=%d).",
				msg->header.message_type, msg->header.payload_length,
				msg->header.timestamp, msg->header.stream_id);
			srs_freep(msg);
			continue;
		}
		
		if ((ret = on_recv_message(msg)) != ERROR_SUCCESS) {
			srs_error("hook the received msg failed. ret=%d", ret);
			srs_freep(msg);
			return ret;
		}
		
		srs_verbose("get a msg with raw/undecoded payload");
		*pmsg = msg;
		break;
	}
	
	return ret;
}

int SrsProtocol::send_message(ISrsMessage* msg)
{
	int ret = ERROR_SUCCESS;
	
	// free msg whatever return value.
	SrsAutoFree(ISrsMessage, msg, false);
	
	if ((ret = msg->encode_packet()) != ERROR_SUCCESS) {
		srs_error("encode packet to message payload failed. ret=%d", ret);
		return ret;
	}
	srs_info("encode packet to message payload success");

	// p set to current write position,
	// it's ok when payload is NULL and size is 0.
	char* p = (char*)msg->payload;
	
	// always write the header event payload is empty.
	do {
		// generate the header.
		char* pheader = NULL;
		int header_size = 0;
		
		if (p == (char*)msg->payload) {
			// write new chunk stream header, fmt is 0
			pheader = out_header_fmt0;
			*pheader++ = 0x00 | (msg->get_perfer_cid() & 0x3F);
			
		    // chunk message header, 11 bytes
		    // timestamp, 3bytes, big-endian
			if (msg->header.timestamp >= RTMP_EXTENDED_TIMESTAMP) {
		        *pheader++ = 0xFF;
		        *pheader++ = 0xFF;
		        *pheader++ = 0xFF;
			} else {
		        pp = (char*)&msg->header.timestamp; 
		        *pheader++ = pp[2];
		        *pheader++ = pp[1];
		        *pheader++ = pp[0];
			}
			
		    // message_length, 3bytes, big-endian
		    pp = (char*)&msg->header.payload_length;
		    *pheader++ = pp[2];
		    *pheader++ = pp[1];
		    *pheader++ = pp[0];
		    
		    // message_type, 1bytes
		    *pheader++ = msg->header.message_type;
		    
		    // message_length, 3bytes, little-endian
		    pp = (char*)&msg->header.stream_id;
		    *pheader++ = pp[0];
		    *pheader++ = pp[1];
		    *pheader++ = pp[2];
		    *pheader++ = pp[3];
		    
		    // chunk extended timestamp header, 0 or 4 bytes, big-endian
		    if(msg->header.timestamp >= RTMP_EXTENDED_TIMESTAMP){
		        pp = (char*)&msg->header.timestamp;
		        *pheader++ = pp[3];
		        *pheader++ = pp[2];
		        *pheader++ = pp[1];
		        *pheader++ = pp[0];
		    }
			
			header_size = pheader - out_header_fmt0;
			pheader = out_header_fmt0;
		} else {
			// write no message header chunk stream, fmt is 3
			pheader = out_header_fmt3;
			*pheader++ = 0xC0 | (msg->get_perfer_cid() & 0x3F);
		    
		    // chunk extended timestamp header, 0 or 4 bytes, big-endian
		    // 6.1.3. Extended Timestamp
		    // This field is transmitted only when the normal time stamp in the
		    // chunk message header is set to 0x00ffffff. If normal time stamp is
		    // set to any value less than 0x00ffffff, this field MUST NOT be
		    // present. This field MUST NOT be present if the timestamp field is not
		    // present. Type 3 chunks MUST NOT have this field.
		    // adobe changed for Type3 chunk:
		    //		FMLE always sendout the extended-timestamp,
		    // 		must send the extended-timestamp to FMS,
		    //		must send the extended-timestamp to flash-player.
		    // @see: ngx_rtmp_prepare_message
		    // @see: http://blog.csdn.net/win_lin/article/details/13363699
		    if(msg->header.timestamp >= RTMP_EXTENDED_TIMESTAMP){
		        pp = (char*)&msg->header.timestamp; 
		        *pheader++ = pp[3];
		        *pheader++ = pp[2];
		        *pheader++ = pp[1];
		        *pheader++ = pp[0];
		    }
			
			header_size = pheader - out_header_fmt3;
			pheader = out_header_fmt3;
		}
		
		// sendout header and payload by writev.
		// decrease the sys invoke count to get higher performance.
		int payload_size = msg->size - (p - (char*)msg->payload);
		payload_size = srs_min(payload_size, out_chunk_size);
		
		// send by writev
		iovec iov[2];
		iov[0].iov_base = pheader;
		iov[0].iov_len = header_size;
		iov[1].iov_base = p;
		iov[1].iov_len = payload_size;
		
		ssize_t nwrite;
		if ((ret = skt->writev(iov, 2, &nwrite)) != ERROR_SUCCESS) {
			srs_error("send with writev failed. ret=%d", ret);
			return ret;
		}
		
		// consume sendout bytes when not empty packet.
		if (msg->payload && msg->size > 0) {
			p += payload_size;
		}
	} while (p < (char*)msg->payload + msg->size);
	
	if ((ret = on_send_message(msg)) != ERROR_SUCCESS) {
		srs_error("hook the send message failed. ret=%d", ret);
		return ret;
	}
	
	return ret;
}

int SrsProtocol::response_acknowledgement_message()
{
	int ret = ERROR_SUCCESS;
	
	SrsCommonMessage* msg = new SrsCommonMessage();
	SrsAcknowledgementPacket* pkt = new SrsAcknowledgementPacket();
	
	in_ack_size.acked_size = pkt->sequence_number = skt->get_recv_bytes();
	msg->set_packet(pkt, 0);
	
	if ((ret = send_message(msg)) != ERROR_SUCCESS) {
		srs_error("send acknowledgement failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("send acknowledgement success.");
	
	return ret;
}

int SrsProtocol::response_ping_message(int32_t timestamp)
{
	int ret = ERROR_SUCCESS;
	
	srs_trace("get a ping request, response it. timestamp=%d", timestamp);
	
	SrsCommonMessage* msg = new SrsCommonMessage();
	SrsUserControlPacket* pkt = new SrsUserControlPacket();
	
	pkt->event_type = SrcPCUCPingResponse;
	pkt->event_data = timestamp;
	msg->set_packet(pkt, 0);
	
	if ((ret = send_message(msg)) != ERROR_SUCCESS) {
		srs_error("send ping response failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("send ping response success.");
	
	return ret;
}

int SrsProtocol::on_recv_message(SrsCommonMessage* msg)
{
	int ret = ERROR_SUCCESS;
	
	srs_assert(msg != NULL);
		
	// acknowledgement
	if (in_ack_size.ack_window_size > 0 && skt->get_recv_bytes() - in_ack_size.acked_size > in_ack_size.ack_window_size) {
		if ((ret = response_acknowledgement_message()) != ERROR_SUCCESS) {
			return ret;
		}
	}
	
	switch (msg->header.message_type) {
		case RTMP_MSG_SetChunkSize:
		case RTMP_MSG_UserControlMessage:
		case RTMP_MSG_WindowAcknowledgementSize:
			if ((ret = msg->decode_packet(this)) != ERROR_SUCCESS) {
				srs_error("decode packet from message payload failed. ret=%d", ret);
				return ret;
			}
			srs_verbose("decode packet from message payload success.");
			break;
	}
	
	switch (msg->header.message_type) {
		case RTMP_MSG_WindowAcknowledgementSize: {
			SrsSetWindowAckSizePacket* pkt = dynamic_cast<SrsSetWindowAckSizePacket*>(msg->get_packet());
			srs_assert(pkt != NULL);
			
			if (pkt->ackowledgement_window_size > 0) {
				in_ack_size.ack_window_size = pkt->ackowledgement_window_size;
				srs_trace("set ack window size to %d", pkt->ackowledgement_window_size);
			} else {
				srs_warn("ignored. set ack window size is %d", pkt->ackowledgement_window_size);
			}
			break;
		}
		case RTMP_MSG_SetChunkSize: {
			SrsSetChunkSizePacket* pkt = dynamic_cast<SrsSetChunkSizePacket*>(msg->get_packet());
			srs_assert(pkt != NULL);
			
			in_chunk_size = pkt->chunk_size;
			
			srs_trace("set input chunk size to %d", pkt->chunk_size);
			break;
		}
		case RTMP_MSG_UserControlMessage: {
			SrsUserControlPacket* pkt = dynamic_cast<SrsUserControlPacket*>(msg->get_packet());
			srs_assert(pkt != NULL);
			
			if (pkt->event_type == SrcPCUCSetBufferLength) {
				srs_trace("ignored. set buffer length to %d", pkt->extra_data);
			}
			if (pkt->event_type == SrcPCUCPingRequest) {
				if ((ret = response_ping_message(pkt->event_data)) != ERROR_SUCCESS) {
					return ret;
				}
			}
			break;
		}
	}
	
	return ret;
}

int SrsProtocol::on_send_message(ISrsMessage* msg)
{
	int ret = ERROR_SUCCESS;
	
	if (!msg->can_decode()) {
		srs_verbose("ignore the un-decodable message.");
		return ret;
	}
	
	SrsCommonMessage* common_msg = dynamic_cast<SrsCommonMessage*>(msg);
	if (!msg) {
		srs_verbose("ignore the shared ptr message.");
		return ret;
	}
	
	srs_assert(common_msg != NULL);
	
	switch (common_msg->header.message_type) {
		case RTMP_MSG_SetChunkSize: {
			SrsSetChunkSizePacket* pkt = dynamic_cast<SrsSetChunkSizePacket*>(common_msg->get_packet());
			srs_assert(pkt != NULL);
			
			out_chunk_size = pkt->chunk_size;
			
			srs_trace("set output chunk size to %d", pkt->chunk_size);
			break;
		}
		case RTMP_MSG_AMF0CommandMessage:
		case RTMP_MSG_AMF3CommandMessage: {
			if (true) {
				SrsConnectAppPacket* pkt = NULL;
				pkt = dynamic_cast<SrsConnectAppPacket*>(common_msg->get_packet());
				if (pkt) {
					requests[pkt->transaction_id] = RTMP_AMF0_COMMAND_CONNECT;
					break;
				}
			}
			if (true) {
				SrsCreateStreamPacket* pkt = NULL;
				pkt = dynamic_cast<SrsCreateStreamPacket*>(common_msg->get_packet());
				if (pkt) {
					requests[pkt->transaction_id] = RTMP_AMF0_COMMAND_CREATE_STREAM;
					break;
				}
			}
			break;
		}
	}
	
	return ret;
}

int SrsProtocol::recv_interlaced_message(SrsCommonMessage** pmsg)
{
	int ret = ERROR_SUCCESS;
	
	// chunk stream basic header.
	char fmt = 0;
	int cid = 0;
	int bh_size = 0;
	if ((ret = read_basic_header(fmt, cid, bh_size)) != ERROR_SUCCESS) {
		if (ret != ERROR_SOCKET_TIMEOUT) {
			srs_error("read basic header failed. ret=%d", ret);
		}
		return ret;
	}
	srs_verbose("read basic header success. fmt=%d, cid=%d, bh_size=%d", fmt, cid, bh_size);
	
	// once we got the chunk message header, 
	// that is there is a real message in cache,
	// increase the timeout to got it.
	// For example, in the play loop, we set timeout to 100ms,
	// when we got a chunk header, we should increase the timeout,
	// or we maybe timeout and disconnect the client.
	int64_t timeout_us = skt->get_recv_timeout();
	if (timeout_us != (int64_t)ST_UTIME_NO_TIMEOUT) {
		int64_t pkt_timeout_us = srs_max(timeout_us, SRS_MIN_RECV_TIMEOUT_US);
		skt->set_recv_timeout(pkt_timeout_us);
		srs_verbose("change recv timeout_us "
			"from %"PRId64" to %"PRId64"", timeout_us, pkt_timeout_us);
	}
	
	// get the cached chunk stream.
	SrsChunkStream* chunk = NULL;
	
	if (chunk_streams.find(cid) == chunk_streams.end()) {
		chunk = chunk_streams[cid] = new SrsChunkStream(cid);
		srs_verbose("cache new chunk stream: fmt=%d, cid=%d", fmt, cid);
	} else {
		chunk = chunk_streams[cid];
		srs_verbose("cached chunk stream: fmt=%d, cid=%d, size=%d, message(type=%d, size=%d, time=%d, sid=%d)",
			chunk->fmt, chunk->cid, (chunk->msg? chunk->msg->size : 0), chunk->header.message_type, chunk->header.payload_length,
			chunk->header.timestamp, chunk->header.stream_id);
	}

	// chunk stream message header
	int mh_size = 0;
	if ((ret = read_message_header(chunk, fmt, bh_size, mh_size)) != ERROR_SUCCESS) {
		if (ret != ERROR_SOCKET_TIMEOUT) {
			srs_error("read message header failed. ret=%d", ret);
		}
		return ret;
	}
	srs_verbose("read message header success. "
			"fmt=%d, mh_size=%d, ext_time=%d, size=%d, message(type=%d, size=%d, time=%d, sid=%d)", 
			fmt, mh_size, chunk->extended_timestamp, (chunk->msg? chunk->msg->size : 0), chunk->header.message_type, 
			chunk->header.payload_length, chunk->header.timestamp, chunk->header.stream_id);
	
	// read msg payload from chunk stream.
	SrsCommonMessage* msg = NULL;
	int payload_size = 0;
	if ((ret = read_message_payload(chunk, bh_size, mh_size, payload_size, &msg)) != ERROR_SUCCESS) {
		if (ret != ERROR_SOCKET_TIMEOUT) {
			srs_error("read message payload failed. ret=%d", ret);
		}
		return ret;
	}
	
	// reset the recv timeout
	if (timeout_us != (int64_t)ST_UTIME_NO_TIMEOUT) {
		skt->set_recv_timeout(timeout_us);
		srs_verbose("reset recv timeout_us to %"PRId64"", timeout_us);
	}
	
	// not got an entire RTMP message, try next chunk.
	if (!msg) {
		srs_verbose("get partial message success. chunk_payload_size=%d, size=%d, message(type=%d, size=%d, time=%d, sid=%d)",
				payload_size, (msg? msg->size : (chunk->msg? chunk->msg->size : 0)), chunk->header.message_type, chunk->header.payload_length,
				chunk->header.timestamp, chunk->header.stream_id);
		return ret;
	}
	
	*pmsg = msg;
	srs_info("get entire message success. chunk_payload_size=%d, size=%d, message(type=%d, size=%d, time=%d, sid=%d)",
			payload_size, (msg? msg->size : (chunk->msg? chunk->msg->size : 0)), chunk->header.message_type, chunk->header.payload_length,
			chunk->header.timestamp, chunk->header.stream_id);
			
	return ret;
}

int SrsProtocol::read_basic_header(char& fmt, int& cid, int& bh_size)
{
	int ret = ERROR_SUCCESS;
	
	int required_size = 1;
	if ((ret = buffer->ensure_buffer_bytes(skt, required_size)) != ERROR_SUCCESS) {
		if (ret != ERROR_SOCKET_TIMEOUT) {
			srs_error("read 1bytes basic header failed. required_size=%d, ret=%d", required_size, ret);
		}
		return ret;
	}
	
	char* p = buffer->bytes();
	
    fmt = (*p >> 6) & 0x03;
    cid = *p & 0x3f;
    bh_size = 1;
    
    if (cid > 1) {
		srs_verbose("%dbytes basic header parsed. fmt=%d, cid=%d", bh_size, fmt, cid);
        return ret;
    }

	if (cid == 0) {
		required_size = 2;
		if ((ret = buffer->ensure_buffer_bytes(skt, required_size)) != ERROR_SUCCESS) {
			if (ret != ERROR_SOCKET_TIMEOUT) {
				srs_error("read 2bytes basic header failed. required_size=%d, ret=%d", required_size, ret);
			}
			return ret;
		}
		
		cid = 64;
		cid += *(++p);
    	bh_size = 2;
		srs_verbose("%dbytes basic header parsed. fmt=%d, cid=%d", bh_size, fmt, cid);
	} else if (cid == 1) {
		required_size = 3;
		if ((ret = buffer->ensure_buffer_bytes(skt, 3)) != ERROR_SUCCESS) {
			if (ret != ERROR_SOCKET_TIMEOUT) {
				srs_error("read 3bytes basic header failed. required_size=%d, ret=%d", required_size, ret);
			}
			return ret;
		}
		
		cid = 64;
		cid += *(++p);
		cid += *(++p) * 256;
    	bh_size = 3;
		srs_verbose("%dbytes basic header parsed. fmt=%d, cid=%d", bh_size, fmt, cid);
	} else {
		srs_error("invalid path, impossible basic header.");
		srs_assert(false);
	}
	
	return ret;
}

int SrsProtocol::read_message_header(SrsChunkStream* chunk, char fmt, int bh_size, int& mh_size)
{
	int ret = ERROR_SUCCESS;
	
	/**
	* we should not assert anything about fmt, for the first packet.
	* (when first packet, the chunk->msg is NULL).
	* the fmt maybe 0/1/2/3, the FMLE will send a 0xC4 for some audio packet.
	* the previous packet is:
	* 	04 			// fmt=0, cid=4
	* 	00 00 1a 	// timestamp=26
	*	00 00 9d 	// payload_length=157
	* 	08 			// message_type=8(audio)
	* 	01 00 00 00 // stream_id=1
	* the current packet maybe:
	* 	c4 			// fmt=3, cid=4
	* it's ok, for the packet is audio, and timestamp delta is 26.
	* the current packet must be parsed as:
	* 	fmt=0, cid=4
	* 	timestamp=26+26=52
	* 	payload_length=157
	* 	message_type=8(audio)
	* 	stream_id=1
	* so we must update the timestamp even fmt=3 for first packet.
	*/
	// fresh packet used to update the timestamp even fmt=3 for first packet.
	bool is_fresh_packet = !chunk->msg;
	
	// but, we can ensure that when a chunk stream is fresh, 
	// the fmt must be 0, a new stream.
	if (chunk->msg_count == 0 && fmt != RTMP_FMT_TYPE0) {
		ret = ERROR_RTMP_CHUNK_START;
		srs_error("chunk stream is fresh, "
			"fmt must be %d, actual is %d. ret=%d", RTMP_FMT_TYPE0, fmt, ret);
		return ret;
	}

	// when exists cache msg, means got an partial message,
	// the fmt must not be type0 which means new message.
	if (chunk->msg && fmt == RTMP_FMT_TYPE0) {
		ret = ERROR_RTMP_CHUNK_START;
		srs_error("chunk stream exists, "
			"fmt must not be %d, actual is %d. ret=%d", RTMP_FMT_TYPE0, fmt, ret);
		return ret;
	}
	
	// create msg when new chunk stream start
	if (!chunk->msg) {
		chunk->msg = new SrsCommonMessage();
		srs_verbose("create message for new chunk, fmt=%d, cid=%d", fmt, chunk->cid);
	}

	// read message header from socket to buffer.
	static char mh_sizes[] = {11, 7, 3, 0};
	mh_size = mh_sizes[(int)fmt];
	srs_verbose("calc chunk message header size. fmt=%d, mh_size=%d", fmt, mh_size);
	
	int required_size = bh_size + mh_size;
	if ((ret = buffer->ensure_buffer_bytes(skt, required_size)) != ERROR_SUCCESS) {
		if (ret != ERROR_SOCKET_TIMEOUT) {
			srs_error("read %dbytes message header failed. required_size=%d, ret=%d", mh_size, required_size, ret);
		}
		return ret;
	}
	char* p = buffer->bytes() + bh_size;
	
	// parse the message header.
	// see also: ngx_rtmp_recv
	if (fmt <= RTMP_FMT_TYPE2) {
        char* pp = (char*)&chunk->header.timestamp_delta;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
        pp[3] = 0;
        
        // fmt: 0
        // timestamp: 3 bytes
        // If the timestamp is greater than or equal to 16777215
        // (hexadecimal 0x00ffffff), this value MUST be 16777215, and the
        // ‘extended timestamp header’ MUST be present. Otherwise, this value
        // SHOULD be the entire timestamp.
        //
        // fmt: 1 or 2
        // timestamp delta: 3 bytes
        // If the delta is greater than or equal to 16777215 (hexadecimal
        // 0x00ffffff), this value MUST be 16777215, and the ‘extended
        // timestamp header’ MUST be present. Otherwise, this value SHOULD be
        // the entire delta.
        chunk->extended_timestamp = (chunk->header.timestamp_delta >= RTMP_EXTENDED_TIMESTAMP);
        if (chunk->extended_timestamp) {
            // Extended timestamp: 0 or 4 bytes
            // This field MUST be sent when the normal timsestamp is set to
            // 0xffffff, it MUST NOT be sent if the normal timestamp is set to
            // anything else. So for values less than 0xffffff the normal
            // timestamp field SHOULD be used in which case the extended timestamp
            // MUST NOT be present. For values greater than or equal to 0xffffff
            // the normal timestamp field MUST NOT be used and MUST be set to
            // 0xffffff and the extended timestamp MUST be sent.
			//
            // if extended timestamp, the timestamp must >= RTMP_EXTENDED_TIMESTAMP
            // we set the timestamp to RTMP_EXTENDED_TIMESTAMP to identify we
            // got an extended timestamp.
			chunk->header.timestamp = RTMP_EXTENDED_TIMESTAMP;
        } else {
	        if (fmt == RTMP_FMT_TYPE0) {
				// 6.1.2.1. Type 0
				// For a type-0 chunk, the absolute timestamp of the message is sent
				// here.
	            chunk->header.timestamp = chunk->header.timestamp_delta;
	        } else {
	            // 6.1.2.2. Type 1
	            // 6.1.2.3. Type 2
	            // For a type-1 or type-2 chunk, the difference between the previous
	            // chunk's timestamp and the current chunk's timestamp is sent here.
	            chunk->header.timestamp += chunk->header.timestamp_delta;
	        }
        }
        
        if (fmt <= RTMP_FMT_TYPE1) {
            pp = (char*)&chunk->header.payload_length;
            pp[2] = *p++;
            pp[1] = *p++;
            pp[0] = *p++;
            pp[3] = 0;
            
            // if msg exists in cache, the size must not changed.
            if (chunk->msg->size > 0 && chunk->msg->size != chunk->header.payload_length) {
                ret = ERROR_RTMP_PACKET_SIZE;
                srs_error("msg exists in chunk cache, "
                	"size=%d cannot change to %d, ret=%d", 
                	chunk->msg->size, chunk->header.payload_length, ret);
                return ret;
            }
            
            chunk->header.message_type = *p++;
            
            if (fmt == 0) {
                pp = (char*)&chunk->header.stream_id;
                pp[0] = *p++;
                pp[1] = *p++;
                pp[2] = *p++;
                pp[3] = *p++;
				srs_verbose("header read completed. fmt=%d, mh_size=%d, ext_time=%d, time=%d, payload=%d, type=%d, sid=%d", 
					fmt, mh_size, chunk->extended_timestamp, chunk->header.timestamp, chunk->header.payload_length, 
					chunk->header.message_type, chunk->header.stream_id);
			} else {
				srs_verbose("header read completed. fmt=%d, mh_size=%d, ext_time=%d, time=%d, payload=%d, type=%d", 
					fmt, mh_size, chunk->extended_timestamp, chunk->header.timestamp, chunk->header.payload_length, 
					chunk->header.message_type);
			}
		} else {
			srs_verbose("header read completed. fmt=%d, mh_size=%d, ext_time=%d, time=%d", 
				fmt, mh_size, chunk->extended_timestamp, chunk->header.timestamp);
		}
	} else {
		// update the timestamp even fmt=3 for first stream
		if (is_fresh_packet && !chunk->extended_timestamp) {
			chunk->header.timestamp += chunk->header.timestamp_delta;
		}
		srs_verbose("header read completed. fmt=%d, size=%d, ext_time=%d", 
			fmt, mh_size, chunk->extended_timestamp);
	}
	
	if (chunk->extended_timestamp) {
		mh_size += 4;
		required_size = bh_size + mh_size;
		srs_verbose("read header ext time. fmt=%d, ext_time=%d, mh_size=%d", fmt, chunk->extended_timestamp, mh_size);
		if ((ret = buffer->ensure_buffer_bytes(skt, required_size)) != ERROR_SUCCESS) {
			if (ret != ERROR_SOCKET_TIMEOUT) {
				srs_error("read %dbytes message header failed. required_size=%d, ret=%d", mh_size, required_size, ret);
			}
			return ret;
		}

		// ffmpeg/librtmp may donot send this filed, need to detect the value.
		// @see also: http://blog.csdn.net/win_lin/article/details/13363699
		int32_t timestamp = 0x00;
        char* pp = (char*)&timestamp;
        pp[3] = *p++;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
        
        // compare to the chunk timestamp, which is set by chunk message header
        // type 0,1 or 2.
        int32_t chunk_timestamp = chunk->header.timestamp;
        if (chunk_timestamp > RTMP_EXTENDED_TIMESTAMP && chunk_timestamp != timestamp) {
            mh_size -= 4;
            srs_verbose("ignore the 4bytes extended timestamp. mh_size=%d", mh_size);
        } else {
            chunk->header.timestamp = timestamp;
        }
		srs_verbose("header read ext_time completed. time=%d", chunk->header.timestamp);
	}
	
	// valid message
	if (chunk->header.payload_length < 0) {
		ret = ERROR_RTMP_MSG_INVLIAD_SIZE;
		srs_error("RTMP message size must not be negative. size=%d, ret=%d", 
			chunk->header.payload_length, ret);
		return ret;
	}
	
	// copy header to msg
	chunk->msg->header = chunk->header;
	
	// increase the msg count, the chunk stream can accept fmt=1/2/3 message now.
	chunk->msg_count++;
	
	return ret;
}

int SrsProtocol::read_message_payload(SrsChunkStream* chunk, int bh_size, int mh_size, int& payload_size, SrsCommonMessage** pmsg)
{
	int ret = ERROR_SUCCESS;
	
	// empty message
	if (chunk->header.payload_length <= 0) {
		// need erase the header in buffer.
		buffer->erase(bh_size + mh_size);
		
		srs_trace("get an empty RTMP "
				"message(type=%d, size=%d, time=%d, sid=%d)", chunk->header.message_type, 
				chunk->header.payload_length, chunk->header.timestamp, chunk->header.stream_id);
		
		*pmsg = chunk->msg;
		chunk->msg = NULL;
				
		return ret;
	}
	srs_assert(chunk->header.payload_length > 0);
	
	// the chunk payload size.
	payload_size = chunk->header.payload_length - chunk->msg->size;
	payload_size = srs_min(payload_size, in_chunk_size);
	srs_verbose("chunk payload size is %d, message_size=%d, received_size=%d, in_chunk_size=%d", 
		payload_size, chunk->header.payload_length, chunk->msg->size, in_chunk_size);

	// create msg payload if not initialized
	if (!chunk->msg->payload) {
		chunk->msg->payload = new int8_t[chunk->header.payload_length];
		memset(chunk->msg->payload, 0, chunk->header.payload_length);
		srs_verbose("create empty payload for RTMP message. size=%d", chunk->header.payload_length);
	}
	
	// read payload to buffer
	int required_size = bh_size + mh_size + payload_size;
	if ((ret = buffer->ensure_buffer_bytes(skt, required_size)) != ERROR_SUCCESS) {
		if (ret != ERROR_SOCKET_TIMEOUT) {
			srs_error("read payload failed. required_size=%d, ret=%d", required_size, ret);
		}
		return ret;
	}
	memcpy(chunk->msg->payload + chunk->msg->size, buffer->bytes() + bh_size + mh_size, payload_size);
	buffer->erase(bh_size + mh_size + payload_size);
	chunk->msg->size += payload_size;
	
	srs_verbose("chunk payload read completed. bh_size=%d, mh_size=%d, payload_size=%d", bh_size, mh_size, payload_size);
	
	// got entire RTMP message?
	if (chunk->header.payload_length == chunk->msg->size) {
		*pmsg = chunk->msg;
		chunk->msg = NULL;
		srs_verbose("get entire RTMP message(type=%d, size=%d, time=%d, sid=%d)", 
				chunk->header.message_type, chunk->header.payload_length, 
				chunk->header.timestamp, chunk->header.stream_id);
		return ret;
	}
	
	srs_verbose("get partial RTMP message(type=%d, size=%d, time=%d, sid=%d), partial size=%d", 
			chunk->header.message_type, chunk->header.payload_length, 
			chunk->header.timestamp, chunk->header.stream_id,
			chunk->msg->size);
	
	return ret;
}

SrsMessageHeader::SrsMessageHeader()
{
	message_type = 0;
	payload_length = 0;
	timestamp_delta = 0;
	stream_id = 0;
	
	timestamp = 0;
}

SrsMessageHeader::~SrsMessageHeader()
{
}

bool SrsMessageHeader::is_audio()
{
	return message_type == RTMP_MSG_AudioMessage;
}

bool SrsMessageHeader::is_video()
{
	return message_type == RTMP_MSG_VideoMessage;
}

bool SrsMessageHeader::is_amf0_command()
{
	return message_type == RTMP_MSG_AMF0CommandMessage;
}

bool SrsMessageHeader::is_amf0_data()
{
	return message_type == RTMP_MSG_AMF0DataMessage;
}

bool SrsMessageHeader::is_amf3_command()
{
	return message_type == RTMP_MSG_AMF3CommandMessage;
}

bool SrsMessageHeader::is_amf3_data()
{
	return message_type == RTMP_MSG_AMF3DataMessage;
}

bool SrsMessageHeader::is_window_ackledgement_size()
{
	return message_type == RTMP_MSG_WindowAcknowledgementSize;
}

bool SrsMessageHeader::is_set_chunk_size()
{
	return message_type == RTMP_MSG_SetChunkSize;
}

bool SrsMessageHeader::is_user_control_message()
{
	return message_type == RTMP_MSG_UserControlMessage;
}

SrsChunkStream::SrsChunkStream(int _cid)
{
	fmt = 0;
	cid = _cid;
	extended_timestamp = false;
	msg = NULL;
	msg_count = 0;
}

SrsChunkStream::~SrsChunkStream()
{
	srs_freep(msg);
}

ISrsMessage::ISrsMessage()
{
	payload = NULL;
	size = 0;
}

ISrsMessage::~ISrsMessage()
{	
}

SrsCommonMessage::SrsCommonMessage()
{
	stream = NULL;
	packet = NULL;
}

SrsCommonMessage::~SrsCommonMessage()
{	
	// we must directly free the ptrs,
	// nevery use the virtual functions to delete,
	// for in the destructor, the virtual functions is disabled.
	
	srs_freepa(payload);
	srs_freep(packet);
	srs_freep(stream);
}

bool SrsCommonMessage::can_decode()
{
	return true;
}

int SrsCommonMessage::decode_packet(SrsProtocol* protocol)
{
	int ret = ERROR_SUCCESS;
	
	srs_assert(payload != NULL);
	srs_assert(size > 0);
	
	if (packet) {
		srs_verbose("msg already decoded");
		return ret;
	}
	
	if (!stream) {
		srs_verbose("create decode stream for message.");
		stream = new SrsStream();
	}
	
	// initialize the decode stream for all message,
	// it's ok for the initialize if fast and without memory copy.
	if ((ret = stream->initialize((char*)payload, size)) != ERROR_SUCCESS) {
		srs_error("initialize stream failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("decode stream initialized success");
	
	// decode specified packet type
	if (header.is_amf0_command() || header.is_amf3_command() || header.is_amf0_data() || header.is_amf3_data()) {
		srs_verbose("start to decode AMF0/AMF3 command message.");
		
		// skip 1bytes to decode the amf3 command.
		if (header.is_amf3_command() && stream->require(1)) {
			srs_verbose("skip 1bytes to decode AMF3 command");
			stream->skip(1);
		}
		
		// amf0 command message.
		// need to read the command name.
		std::string command;
		if ((ret = srs_amf0_read_string(stream, command)) != ERROR_SUCCESS) {
			srs_error("decode AMF0/AMF3 command name failed. ret=%d", ret);
			return ret;
		}
		srs_verbose("AMF0/AMF3 command message, command_name=%s", command.c_str());
		
		// result/error packet
		if (command == RTMP_AMF0_COMMAND_RESULT || command == RTMP_AMF0_COMMAND_ERROR) {
			double transactionId = 0.0;
			if ((ret = srs_amf0_read_number(stream, transactionId)) != ERROR_SUCCESS) {
				srs_error("decode AMF0/AMF3 transcationId failed. ret=%d", ret);
				return ret;
			}
			srs_verbose("AMF0/AMF3 command id, transcationId=%.2f", transactionId);
			
			// reset stream, for header read completed.
			stream->reset();
			
			std::string request_name = protocol->get_request_name(transactionId);
			if (request_name.empty()) {
				ret = ERROR_RTMP_NO_REQUEST;
				srs_error("decode AMF0/AMF3 request failed. ret=%d", ret);
				return ret;
			}
			srs_verbose("AMF0/AMF3 request parsed. request_name=%s", request_name.c_str());

			if (request_name == RTMP_AMF0_COMMAND_CONNECT) {
				srs_info("decode the AMF0/AMF3 response command(connect vhost/app message).");
				packet = new SrsConnectAppResPacket();
				return packet->decode(stream);
			} else if (request_name == RTMP_AMF0_COMMAND_CREATE_STREAM) {
				srs_info("decode the AMF0/AMF3 response command(createStream message).");
				packet = new SrsCreateStreamResPacket(0, 0);
				return packet->decode(stream);
			} else {
				ret = ERROR_RTMP_NO_REQUEST;
				srs_error("decode AMF0/AMF3 request failed. "
					"request_name=%s, transactionId=%.2f, ret=%d", 
					request_name.c_str(), transactionId, ret);
				return ret;
			}
		}
		
		// reset to zero(amf3 to 1) to restart decode.
		stream->reset();
		if (header.is_amf3_command()) {
			stream->skip(1);
		}
		
		// decode command object.
		if (command == RTMP_AMF0_COMMAND_CONNECT) {
			srs_info("decode the AMF0/AMF3 command(connect vhost/app message).");
			packet = new SrsConnectAppPacket();
			return packet->decode(stream);
		} else if(command == RTMP_AMF0_COMMAND_CREATE_STREAM) {
			srs_info("decode the AMF0/AMF3 command(createStream message).");
			packet = new SrsCreateStreamPacket();
			return packet->decode(stream);
		} else if(command == RTMP_AMF0_COMMAND_PLAY) {
			srs_info("decode the AMF0/AMF3 command(paly message).");
			packet = new SrsPlayPacket();
			return packet->decode(stream);
		} else if(command == RTMP_AMF0_COMMAND_PAUSE) {
			srs_info("decode the AMF0/AMF3 command(pause message).");
			packet = new SrsPausePacket();
			return packet->decode(stream);
		} else if(command == RTMP_AMF0_COMMAND_RELEASE_STREAM) {
			srs_info("decode the AMF0/AMF3 command(FMLE releaseStream message).");
			packet = new SrsFMLEStartPacket();
			return packet->decode(stream);
		} else if(command == RTMP_AMF0_COMMAND_FC_PUBLISH) {
			srs_info("decode the AMF0/AMF3 command(FMLE FCPublish message).");
			packet = new SrsFMLEStartPacket();
			return packet->decode(stream);
		} else if(command == RTMP_AMF0_COMMAND_PUBLISH) {
			srs_info("decode the AMF0/AMF3 command(publish message).");
			packet = new SrsPublishPacket();
			return packet->decode(stream);
		} else if(command == RTMP_AMF0_COMMAND_UNPUBLISH) {
			srs_info("decode the AMF0/AMF3 command(unpublish message).");
			packet = new SrsFMLEStartPacket();
			return packet->decode(stream);
		} else if(command == RTMP_AMF0_DATA_SET_DATAFRAME || command == RTMP_AMF0_DATA_ON_METADATA) {
			srs_info("decode the AMF0/AMF3 data(onMetaData message).");
			packet = new SrsOnMetaDataPacket();
			return packet->decode(stream);
		}
		
		// default packet to drop message.
		srs_trace("drop the AMF0/AMF3 command message, command_name=%s", command.c_str());
		packet = new SrsPacket();
		return ret;
	} else if(header.is_user_control_message()) {
		srs_verbose("start to decode user control message.");
		packet = new SrsUserControlPacket();
		return packet->decode(stream);
	} else if(header.is_window_ackledgement_size()) {
		srs_verbose("start to decode set ack window size message.");
		packet = new SrsSetWindowAckSizePacket();
		return packet->decode(stream);
	} else if(header.is_set_chunk_size()) {
		srs_verbose("start to decode set chunk size message.");
		packet = new SrsSetChunkSizePacket();
		return packet->decode(stream);
	} else {
		// default packet to drop message.
		srs_trace("drop the unknown message, type=%d", header.message_type);
		packet = new SrsPacket();
	}
	
	return ret;
}

SrsPacket* SrsCommonMessage::get_packet()
{
	if (!packet) {
		srs_error("the payload is raw/undecoded, invoke decode_packet to decode it.");
	}
	srs_assert(packet != NULL);
	
	return packet;
}

int SrsCommonMessage::get_perfer_cid()
{
	if (!packet) {
		return RTMP_CID_ProtocolControl;
	}
	
	// we donot use the complex basic header,
	// ensure the basic header is 1bytes.
	if (packet->get_perfer_cid() < 2) {
		return packet->get_perfer_cid();
	}
	
	return packet->get_perfer_cid();
}

void SrsCommonMessage::set_packet(SrsPacket* pkt, int stream_id)
{
	srs_freep(packet);

	packet = pkt;
	
	header.message_type = packet->get_message_type();
	header.payload_length = packet->get_payload_length();
	header.stream_id = stream_id;
}

int SrsCommonMessage::encode_packet()
{
	int ret = ERROR_SUCCESS;
	
	if (packet == NULL) {
		srs_warn("packet is empty, send out empty message.");
		return ret;
	}
	// realloc the payload.
	size = 0;
	srs_freepa(payload);
	
	if ((ret = packet->encode(size, (char*&)payload)) != ERROR_SUCCESS) {
		return ret;
	}
	
	header.payload_length = size;
	
	return ret;
}

SrsSharedPtrMessage::SrsSharedPtr::SrsSharedPtr()
{
	payload = NULL;
	size = 0;
	perfer_cid = 0;
	shared_count = 0;
}

SrsSharedPtrMessage::SrsSharedPtr::~SrsSharedPtr()
{
	srs_freepa(payload);
}

SrsSharedPtrMessage::SrsSharedPtrMessage()
{
	ptr = NULL;
}

SrsSharedPtrMessage::~SrsSharedPtrMessage()
{
	if (ptr) {
		if (ptr->shared_count == 0) {
			srs_freep(ptr);
		} else {
			ptr->shared_count--;
		}
	}
}

bool SrsSharedPtrMessage::can_decode()
{
	return false;
}

int SrsSharedPtrMessage::initialize(SrsCommonMessage* source)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = initialize(source, (char*)source->payload, source->size)) != ERROR_SUCCESS) {
		return ret;
	}
	
	// detach the payload from source
	source->payload = NULL;
	source->size = 0;
	
	return ret;
}

int SrsSharedPtrMessage::initialize(SrsCommonMessage* source, char* payload, int size)
{
	int ret = ERROR_SUCCESS;
	
	srs_assert(source != NULL);
	if (ptr) {
		ret = ERROR_SYSTEM_ASSERT_FAILED;
		srs_error("should not set the payload twice. ret=%d", ret);
		srs_assert(false);
		
		return ret;
	}
	
	header = source->header;
	header.payload_length = size;
	
	ptr = new SrsSharedPtr();
	
	// direct attach the data of common message.
	ptr->payload = payload;
	ptr->size = size;
	
	if (source->header.is_video()) {
		ptr->perfer_cid = RTMP_CID_Video;
	} else if (source->header.is_audio()) {
		ptr->perfer_cid = RTMP_CID_Audio;
	} else {
		ptr->perfer_cid = RTMP_CID_OverConnection2;
	}
	
	super::payload = (int8_t*)ptr->payload;
	super::size = ptr->size;
	
	return ret;
}

SrsSharedPtrMessage* SrsSharedPtrMessage::copy()
{
	if (!ptr) {
		srs_error("invoke initialize to initialize the ptr.");
		srs_assert(false);
		return NULL;
	}
	
	SrsSharedPtrMessage* copy = new SrsSharedPtrMessage();
	
	copy->header = header;
	
	copy->ptr = ptr;
	ptr->shared_count++;
	
	copy->payload = (int8_t*)ptr->payload;
	copy->size = ptr->size;
	
	return copy;
}

int SrsSharedPtrMessage::get_perfer_cid()
{
	if (!ptr) {
		return 0;
	}
	
	return ptr->perfer_cid;
}

int SrsSharedPtrMessage::encode_packet()
{
	srs_verbose("shared message ignore the encode method.");
	return ERROR_SUCCESS;
}

SrsPacket::SrsPacket()
{
}

SrsPacket::~SrsPacket()
{
}

int SrsPacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	srs_assert(stream != NULL);

	ret = ERROR_SYSTEM_PACKET_INVALID;
	srs_error("current packet is not support to decode. "
		"paket=%s, ret=%d", get_class_name(), ret);
	
	return ret;
}

int SrsPacket::get_perfer_cid()
{
	return 0;
}

int SrsPacket::get_message_type()
{
	return 0;
}

int SrsPacket::get_payload_length()
{
	return get_size();
}

int SrsPacket::encode(int& psize, char*& ppayload)
{
	int ret = ERROR_SUCCESS;
	
	int size = get_size();
	char* payload = NULL;
	
	SrsStream stream;
	
	if (size > 0) {
		payload = new char[size];
		
		if ((ret = stream.initialize(payload, size)) != ERROR_SUCCESS) {
			srs_error("initialize the stream failed. ret=%d", ret);
			srs_freepa(payload);
			return ret;
		}
	}
	
	if ((ret = encode_packet(&stream)) != ERROR_SUCCESS) {
		srs_error("encode the packet failed. ret=%d", ret);
		srs_freepa(payload);
		return ret;
	}
	
	psize = size;
	ppayload = payload;
	srs_verbose("encode the packet success. size=%d", size);
	
	return ret;
}

int SrsPacket::get_size()
{
	return 0;
}

int SrsPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	srs_assert(stream != NULL);

	ret = ERROR_SYSTEM_PACKET_INVALID;
	srs_error("current packet is not support to encode. "
		"paket=%s, ret=%d", get_class_name(), ret);
	
	return ret;
}

SrsConnectAppPacket::SrsConnectAppPacket()
{
	command_name = RTMP_AMF0_COMMAND_CONNECT;
	transaction_id = 1;
	command_object = NULL;
}

SrsConnectAppPacket::~SrsConnectAppPacket()
{
	srs_freep(command_object);
}

int SrsConnectAppPacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;

	if ((ret = srs_amf0_read_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("amf0 decode connect command_name failed. ret=%d", ret);
		return ret;
	}
	if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_CONNECT) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode connect command_name failed. "
			"command_name=%s, ret=%d", command_name.c_str(), ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("amf0 decode connect transaction_id failed. ret=%d", ret);
		return ret;
	}
	if (transaction_id != 1.0) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode connect transaction_id failed. "
			"required=%.1f, actual=%.1f, ret=%d", 1.0, transaction_id, ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_object(stream, command_object)) != ERROR_SUCCESS) {
		srs_error("amf0 decode connect command_object failed. ret=%d", ret);
		return ret;
	}
	if (command_object == NULL) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode connect command_object failed. ret=%d", ret);
		return ret;
	}
	
	srs_info("amf0 decode connect packet success");
	
	return ret;
}

int SrsConnectAppPacket::get_perfer_cid()
{
	return RTMP_CID_OverConnection;
}

int SrsConnectAppPacket::get_message_type()
{
	return RTMP_MSG_AMF0CommandMessage;
}

int SrsConnectAppPacket::get_size()
{
	return srs_amf0_get_string_size(command_name) + srs_amf0_get_number_size()
		+ srs_amf0_get_object_size(command_object);
}

int SrsConnectAppPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("encode transaction_id failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode transaction_id success.");
	
	if ((ret = srs_amf0_write_object(stream, command_object)) != ERROR_SUCCESS) {
		srs_error("encode command_object failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_object success.");
	
	srs_info("encode connect app request packet success.");
	
	return ret;
}

SrsConnectAppResPacket::SrsConnectAppResPacket()
{
	command_name = RTMP_AMF0_COMMAND_RESULT;
	transaction_id = 1;
	props = new SrsAmf0Object();
	info = new SrsAmf0Object();
}

SrsConnectAppResPacket::~SrsConnectAppResPacket()
{
	srs_freep(props);
	srs_freep(info);
}

int SrsConnectAppResPacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;

	if ((ret = srs_amf0_read_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("amf0 decode connect command_name failed. ret=%d", ret);
		return ret;
	}
	if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_RESULT) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode connect command_name failed. "
			"command_name=%s, ret=%d", command_name.c_str(), ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("amf0 decode connect transaction_id failed. ret=%d", ret);
		return ret;
	}
	if (transaction_id != 1.0) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode connect transaction_id failed. "
			"required=%.1f, actual=%.1f, ret=%d", 1.0, transaction_id, ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_object(stream, props)) != ERROR_SUCCESS) {
		srs_error("amf0 decode connect props failed. ret=%d", ret);
		return ret;
	}
	if (props == NULL) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode connect props failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_object(stream, info)) != ERROR_SUCCESS) {
		srs_error("amf0 decode connect info failed. ret=%d", ret);
		return ret;
	}
	if (info == NULL) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode connect info failed. ret=%d", ret);
		return ret;
	}
	
	srs_info("amf0 decode connect response packet success");
	
	return ret;
}

int SrsConnectAppResPacket::get_perfer_cid()
{
	return RTMP_CID_OverConnection;
}

int SrsConnectAppResPacket::get_message_type()
{
	return RTMP_MSG_AMF0CommandMessage;
}

int SrsConnectAppResPacket::get_size()
{
	return srs_amf0_get_string_size(command_name) + srs_amf0_get_number_size()
		+ srs_amf0_get_object_size(props)+ srs_amf0_get_object_size(info);
}

int SrsConnectAppResPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("encode transaction_id failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode transaction_id success.");
	
	if ((ret = srs_amf0_write_object(stream, props)) != ERROR_SUCCESS) {
		srs_error("encode props failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode props success.");
	
	if ((ret = srs_amf0_write_object(stream, info)) != ERROR_SUCCESS) {
		srs_error("encode info failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode info success.");
	
	srs_info("encode connect app response packet success.");
	
	return ret;
}

SrsCreateStreamPacket::SrsCreateStreamPacket()
{
	command_name = RTMP_AMF0_COMMAND_CREATE_STREAM;
	transaction_id = 2;
	command_object = new SrsAmf0Null();
}

SrsCreateStreamPacket::~SrsCreateStreamPacket()
{
	srs_freep(command_object);
}

int SrsCreateStreamPacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;

	if ((ret = srs_amf0_read_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("amf0 decode createStream command_name failed. ret=%d", ret);
		return ret;
	}
	if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_CREATE_STREAM) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode createStream command_name failed. "
			"command_name=%s, ret=%d", command_name.c_str(), ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("amf0 decode createStream transaction_id failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_null(stream)) != ERROR_SUCCESS) {
		srs_error("amf0 decode createStream command_object failed. ret=%d", ret);
		return ret;
	}
	
	srs_info("amf0 decode createStream packet success");
	
	return ret;
}

int SrsCreateStreamPacket::get_perfer_cid()
{
	return RTMP_CID_OverConnection;
}

int SrsCreateStreamPacket::get_message_type()
{
	return RTMP_MSG_AMF0CommandMessage;
}

int SrsCreateStreamPacket::get_size()
{
	return srs_amf0_get_string_size(command_name) + srs_amf0_get_number_size()
		+ srs_amf0_get_null_size();
}

int SrsCreateStreamPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("encode transaction_id failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode transaction_id success.");
	
	if ((ret = srs_amf0_write_null(stream)) != ERROR_SUCCESS) {
		srs_error("encode command_object failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_object success.");
	
	srs_info("encode create stream request packet success.");
	
	return ret;
}

SrsCreateStreamResPacket::SrsCreateStreamResPacket(double _transaction_id, double _stream_id)
{
	command_name = RTMP_AMF0_COMMAND_RESULT;
	transaction_id = _transaction_id;
	command_object = new SrsAmf0Null();
	stream_id = _stream_id;
}

SrsCreateStreamResPacket::~SrsCreateStreamResPacket()
{
	srs_freep(command_object);
}

int SrsCreateStreamResPacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;

	if ((ret = srs_amf0_read_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("amf0 decode createStream command_name failed. ret=%d", ret);
		return ret;
	}
	if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_RESULT) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode createStream command_name failed. "
			"command_name=%s, ret=%d", command_name.c_str(), ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("amf0 decode createStream transaction_id failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_null(stream)) != ERROR_SUCCESS) {
		srs_error("amf0 decode createStream command_object failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_number(stream, stream_id)) != ERROR_SUCCESS) {
		srs_error("amf0 decode createStream stream_id failed. ret=%d", ret);
		return ret;
	}
	
	srs_info("amf0 decode createStream response packet success");
	
	return ret;
}

int SrsCreateStreamResPacket::get_perfer_cid()
{
	return RTMP_CID_OverConnection;
}

int SrsCreateStreamResPacket::get_message_type()
{
	return RTMP_MSG_AMF0CommandMessage;
}

int SrsCreateStreamResPacket::get_size()
{
	return srs_amf0_get_string_size(command_name) + srs_amf0_get_number_size()
		+ srs_amf0_get_null_size() + srs_amf0_get_number_size();
}

int SrsCreateStreamResPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("encode transaction_id failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode transaction_id success.");
	
	if ((ret = srs_amf0_write_null(stream)) != ERROR_SUCCESS) {
		srs_error("encode command_object failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_object success.");
	
	if ((ret = srs_amf0_write_number(stream, stream_id)) != ERROR_SUCCESS) {
		srs_error("encode stream_id failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode stream_id success.");
	
	
	srs_info("encode createStream response packet success.");
	
	return ret;
}

SrsFMLEStartPacket::SrsFMLEStartPacket()
{
	command_name = RTMP_AMF0_COMMAND_CREATE_STREAM;
	transaction_id = 0;
	command_object = new SrsAmf0Null();
}

SrsFMLEStartPacket::~SrsFMLEStartPacket()
{
	srs_freep(command_object);
}

int SrsFMLEStartPacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;

	if ((ret = srs_amf0_read_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("amf0 decode FMLE start command_name failed. ret=%d", ret);
		return ret;
	}
	if (command_name.empty() 
		|| (command_name != RTMP_AMF0_COMMAND_RELEASE_STREAM 
		&& command_name != RTMP_AMF0_COMMAND_FC_PUBLISH
		&& command_name != RTMP_AMF0_COMMAND_UNPUBLISH)
	) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode FMLE start command_name failed. "
			"command_name=%s, ret=%d", command_name.c_str(), ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("amf0 decode FMLE start transaction_id failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_null(stream)) != ERROR_SUCCESS) {
		srs_error("amf0 decode FMLE start command_object failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_string(stream, stream_name)) != ERROR_SUCCESS) {
		srs_error("amf0 decode FMLE start stream_name failed. ret=%d", ret);
		return ret;
	}
	
	srs_info("amf0 decode FMLE start packet success");
	
	return ret;
}

SrsFMLEStartResPacket::SrsFMLEStartResPacket(double _transaction_id)
{
	command_name = RTMP_AMF0_COMMAND_RESULT;
	transaction_id = _transaction_id;
	command_object = new SrsAmf0Null();
	args = new SrsAmf0Undefined();
}

SrsFMLEStartResPacket::~SrsFMLEStartResPacket()
{
	srs_freep(command_object);
	srs_freep(args);
}

int SrsFMLEStartResPacket::get_perfer_cid()
{
	return RTMP_CID_OverConnection;
}

int SrsFMLEStartResPacket::get_message_type()
{
	return RTMP_MSG_AMF0CommandMessage;
}

int SrsFMLEStartResPacket::get_size()
{
	return srs_amf0_get_string_size(command_name) + srs_amf0_get_number_size()
		+ srs_amf0_get_null_size() + srs_amf0_get_undefined_size();
}

int SrsFMLEStartResPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("encode transaction_id failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode transaction_id success.");
	
	if ((ret = srs_amf0_write_null(stream)) != ERROR_SUCCESS) {
		srs_error("encode command_object failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_object success.");
	
	if ((ret = srs_amf0_write_undefined(stream)) != ERROR_SUCCESS) {
		srs_error("encode args failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode args success.");
	
	
	srs_info("encode FMLE start response packet success.");
	
	return ret;
}

SrsPublishPacket::SrsPublishPacket()
{
	command_name = RTMP_AMF0_COMMAND_PUBLISH;
	transaction_id = 0;
	command_object = new SrsAmf0Null();
	type = "live";
}

SrsPublishPacket::~SrsPublishPacket()
{
	srs_freep(command_object);
}

int SrsPublishPacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;

	if ((ret = srs_amf0_read_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("amf0 decode publish command_name failed. ret=%d", ret);
		return ret;
	}
	if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_PUBLISH) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode publish command_name failed. "
			"command_name=%s, ret=%d", command_name.c_str(), ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("amf0 decode publish transaction_id failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_null(stream)) != ERROR_SUCCESS) {
		srs_error("amf0 decode publish command_object failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_string(stream, stream_name)) != ERROR_SUCCESS) {
		srs_error("amf0 decode publish stream_name failed. ret=%d", ret);
		return ret;
	}
	
	if (!stream->empty() && (ret = srs_amf0_read_string(stream, type)) != ERROR_SUCCESS) {
		srs_error("amf0 decode publish type failed. ret=%d", ret);
		return ret;
	}
	
	srs_info("amf0 decode publish packet success");
	
	return ret;
}

int SrsPublishPacket::get_perfer_cid()
{
	return RTMP_CID_OverStream;
}

int SrsPublishPacket::get_message_type()
{
	return RTMP_MSG_AMF0CommandMessage;
}

int SrsPublishPacket::get_size()
{
	return srs_amf0_get_string_size(command_name) + srs_amf0_get_number_size()
		+ srs_amf0_get_null_size() + srs_amf0_get_string_size(stream_name)
		+ srs_amf0_get_string_size(type);
}

int SrsPublishPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("encode transaction_id failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode transaction_id success.");
	
	if ((ret = srs_amf0_write_null(stream)) != ERROR_SUCCESS) {
		srs_error("encode command_object failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_object success.");
	
	if ((ret = srs_amf0_write_string(stream, stream_name)) != ERROR_SUCCESS) {
		srs_error("encode stream_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode stream_name success.");
	
	if ((ret = srs_amf0_write_string(stream, type)) != ERROR_SUCCESS) {
		srs_error("encode type failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode type success.");
	
	srs_info("encode play request packet success.");
	
	return ret;
}

SrsPausePacket::SrsPausePacket()
{
	command_name = RTMP_AMF0_COMMAND_PAUSE;
	transaction_id = 0;
	command_object = new SrsAmf0Null();

	time_ms = 0;
	is_pause = true;
}

SrsPausePacket::~SrsPausePacket()
{
	srs_freep(command_object);
}

int SrsPausePacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;

	if ((ret = srs_amf0_read_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("amf0 decode pause command_name failed. ret=%d", ret);
		return ret;
	}
	if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_PAUSE) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode pause command_name failed. "
			"command_name=%s, ret=%d", command_name.c_str(), ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("amf0 decode pause transaction_id failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_null(stream)) != ERROR_SUCCESS) {
		srs_error("amf0 decode pause command_object failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_boolean(stream, is_pause)) != ERROR_SUCCESS) {
		srs_error("amf0 decode pause is_pause failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_number(stream, time_ms)) != ERROR_SUCCESS) {
		srs_error("amf0 decode pause time_ms failed. ret=%d", ret);
		return ret;
	}
	
	srs_info("amf0 decode pause packet success");
	
	return ret;
}

SrsPlayPacket::SrsPlayPacket()
{
	command_name = RTMP_AMF0_COMMAND_PLAY;
	transaction_id = 0;
	command_object = new SrsAmf0Null();

	start = -2;
	duration = -1;
	reset = true;
}

SrsPlayPacket::~SrsPlayPacket()
{
	srs_freep(command_object);
}

int SrsPlayPacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;

	if ((ret = srs_amf0_read_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("amf0 decode play command_name failed. ret=%d", ret);
		return ret;
	}
	if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_PLAY) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("amf0 decode play command_name failed. "
			"command_name=%s, ret=%d", command_name.c_str(), ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("amf0 decode play transaction_id failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_null(stream)) != ERROR_SUCCESS) {
		srs_error("amf0 decode play command_object failed. ret=%d", ret);
		return ret;
	}
	
	if ((ret = srs_amf0_read_string(stream, stream_name)) != ERROR_SUCCESS) {
		srs_error("amf0 decode play stream_name failed. ret=%d", ret);
		return ret;
	}
	
	if (!stream->empty() && (ret = srs_amf0_read_number(stream, start)) != ERROR_SUCCESS) {
		srs_error("amf0 decode play start failed. ret=%d", ret);
		return ret;
	}
	if (!stream->empty() && (ret = srs_amf0_read_number(stream, duration)) != ERROR_SUCCESS) {
		srs_error("amf0 decode play duration failed. ret=%d", ret);
		return ret;
	}
	if (!stream->empty() && (ret = srs_amf0_read_boolean(stream, reset)) != ERROR_SUCCESS) {
		srs_error("amf0 decode play reset failed. ret=%d", ret);
		return ret;
	}
	
	srs_info("amf0 decode play packet success");
	
	return ret;
}

int SrsPlayPacket::get_perfer_cid()
{
	return RTMP_CID_OverStream;
}

int SrsPlayPacket::get_message_type()
{
	return RTMP_MSG_AMF0CommandMessage;
}

int SrsPlayPacket::get_size()
{
	return srs_amf0_get_string_size(command_name) + srs_amf0_get_number_size()
		+ srs_amf0_get_null_size() + srs_amf0_get_string_size(stream_name)
		+ srs_amf0_get_number_size() + srs_amf0_get_number_size()
		+ srs_amf0_get_boolean_size();
}

int SrsPlayPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("encode transaction_id failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode transaction_id success.");
	
	if ((ret = srs_amf0_write_null(stream)) != ERROR_SUCCESS) {
		srs_error("encode command_object failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_object success.");
	
	if ((ret = srs_amf0_write_string(stream, stream_name)) != ERROR_SUCCESS) {
		srs_error("encode stream_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode stream_name success.");
	
	if ((ret = srs_amf0_write_number(stream, start)) != ERROR_SUCCESS) {
		srs_error("encode start failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode start success.");
	
	if ((ret = srs_amf0_write_number(stream, duration)) != ERROR_SUCCESS) {
		srs_error("encode duration failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode duration success.");
	
	if ((ret = srs_amf0_write_boolean(stream, reset)) != ERROR_SUCCESS) {
		srs_error("encode reset failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode reset success.");
	
	srs_info("encode play request packet success.");
	
	return ret;
}

SrsPlayResPacket::SrsPlayResPacket()
{
	command_name = RTMP_AMF0_COMMAND_RESULT;
	transaction_id = 0;
	command_object = new SrsAmf0Null();
	desc = new SrsAmf0Object();
}

SrsPlayResPacket::~SrsPlayResPacket()
{
	srs_freep(command_object);
	srs_freep(desc);
}

int SrsPlayResPacket::get_perfer_cid()
{
	return RTMP_CID_OverStream;
}

int SrsPlayResPacket::get_message_type()
{
	return RTMP_MSG_AMF0CommandMessage;
}

int SrsPlayResPacket::get_size()
{
	return srs_amf0_get_string_size(command_name) + srs_amf0_get_number_size()
		+ srs_amf0_get_null_size() + srs_amf0_get_object_size(desc);
}

int SrsPlayResPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("encode transaction_id failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode transaction_id success.");
	
	if ((ret = srs_amf0_write_null(stream)) != ERROR_SUCCESS) {
		srs_error("encode command_object failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_object success.");
	
	if ((ret = srs_amf0_write_object(stream, desc)) != ERROR_SUCCESS) {
		srs_error("encode desc failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode desc success.");
	
	
	srs_info("encode play response packet success.");
	
	return ret;
}

SrsOnBWDonePacket::SrsOnBWDonePacket()
{
	command_name = RTMP_AMF0_COMMAND_ON_BW_DONE;
	transaction_id = 0;
	args = new SrsAmf0Null();
}

SrsOnBWDonePacket::~SrsOnBWDonePacket()
{
	srs_freep(args);
}

int SrsOnBWDonePacket::get_perfer_cid()
{
	return RTMP_CID_OverConnection;
}

int SrsOnBWDonePacket::get_message_type()
{
	return RTMP_MSG_AMF0CommandMessage;
}

int SrsOnBWDonePacket::get_size()
{
	return srs_amf0_get_string_size(command_name) + srs_amf0_get_number_size()
		+ srs_amf0_get_null_size();
}

int SrsOnBWDonePacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("encode transaction_id failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode transaction_id success.");
	
	if ((ret = srs_amf0_write_null(stream)) != ERROR_SUCCESS) {
		srs_error("encode args failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode args success.");
	
	srs_info("encode onBWDone packet success.");
	
	return ret;
}

SrsOnStatusCallPacket::SrsOnStatusCallPacket()
{
	command_name = RTMP_AMF0_COMMAND_ON_STATUS;
	transaction_id = 0;
	args = new SrsAmf0Null();
	data = new SrsAmf0Object();
}

SrsOnStatusCallPacket::~SrsOnStatusCallPacket()
{
	srs_freep(args);
	srs_freep(data);
}

int SrsOnStatusCallPacket::get_perfer_cid()
{
	return RTMP_CID_OverStream;
}

int SrsOnStatusCallPacket::get_message_type()
{
	return RTMP_MSG_AMF0CommandMessage;
}

int SrsOnStatusCallPacket::get_size()
{
	return srs_amf0_get_string_size(command_name) + srs_amf0_get_number_size()
		+ srs_amf0_get_null_size() + srs_amf0_get_object_size(data);
}

int SrsOnStatusCallPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_number(stream, transaction_id)) != ERROR_SUCCESS) {
		srs_error("encode transaction_id failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode transaction_id success.");
	
	if ((ret = srs_amf0_write_null(stream)) != ERROR_SUCCESS) {
		srs_error("encode args failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode args success.");;
	
	if ((ret = srs_amf0_write_object(stream, data)) != ERROR_SUCCESS) {
		srs_error("encode data failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode data success.");
	
	srs_info("encode onStatus(Call) packet success.");
	
	return ret;
}

SrsOnStatusDataPacket::SrsOnStatusDataPacket()
{
	command_name = RTMP_AMF0_COMMAND_ON_STATUS;
	data = new SrsAmf0Object();
}

SrsOnStatusDataPacket::~SrsOnStatusDataPacket()
{
	srs_freep(data);
}

int SrsOnStatusDataPacket::get_perfer_cid()
{
	return RTMP_CID_OverStream;
}

int SrsOnStatusDataPacket::get_message_type()
{
	return RTMP_MSG_AMF0DataMessage;
}

int SrsOnStatusDataPacket::get_size()
{
	return srs_amf0_get_string_size(command_name) + srs_amf0_get_object_size(data);
}

int SrsOnStatusDataPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_object(stream, data)) != ERROR_SUCCESS) {
		srs_error("encode data failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode data success.");
	
	srs_info("encode onStatus(Data) packet success.");
	
	return ret;
}

SrsSampleAccessPacket::SrsSampleAccessPacket()
{
	command_name = RTMP_AMF0_DATA_SAMPLE_ACCESS;
	video_sample_access = false;
	audio_sample_access = false;
}

SrsSampleAccessPacket::~SrsSampleAccessPacket()
{
}

int SrsSampleAccessPacket::get_perfer_cid()
{
	return RTMP_CID_OverStream;
}

int SrsSampleAccessPacket::get_message_type()
{
	return RTMP_MSG_AMF0DataMessage;
}

int SrsSampleAccessPacket::get_size()
{
	return srs_amf0_get_string_size(command_name)
		+ srs_amf0_get_boolean_size() + srs_amf0_get_boolean_size();
}

int SrsSampleAccessPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, command_name)) != ERROR_SUCCESS) {
		srs_error("encode command_name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode command_name success.");
	
	if ((ret = srs_amf0_write_boolean(stream, video_sample_access)) != ERROR_SUCCESS) {
		srs_error("encode video_sample_access failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode video_sample_access success.");
	
	if ((ret = srs_amf0_write_boolean(stream, audio_sample_access)) != ERROR_SUCCESS) {
		srs_error("encode audio_sample_access failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode audio_sample_access success.");;
	
	srs_info("encode |RtmpSampleAccess packet success.");
	
	return ret;
}

SrsOnMetaDataPacket::SrsOnMetaDataPacket()
{
	name = RTMP_AMF0_DATA_ON_METADATA;
	metadata = new SrsAmf0Object();
}

SrsOnMetaDataPacket::~SrsOnMetaDataPacket()
{
	srs_freep(metadata);
}

int SrsOnMetaDataPacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_read_string(stream, name)) != ERROR_SUCCESS) {
		srs_error("decode metadata name failed. ret=%d", ret);
		return ret;
	}

	// ignore the @setDataFrame
	if (name == RTMP_AMF0_DATA_SET_DATAFRAME) {
		if ((ret = srs_amf0_read_string(stream, name)) != ERROR_SUCCESS) {
			srs_error("decode metadata name failed. ret=%d", ret);
			return ret;
		}
	}
	
	srs_verbose("decode metadata name success. name=%s", name.c_str());
	
	// the metadata maybe object or ecma array
	SrsAmf0Any* any = NULL;
	if ((ret = srs_amf0_read_any(stream, any)) != ERROR_SUCCESS) {
		srs_error("decode metadata metadata failed. ret=%d", ret);
		return ret;
	}
	
	if (any->is_object()) {
		srs_freep(metadata);
		metadata = srs_amf0_convert<SrsAmf0Object>(any);
		srs_info("decode metadata object success");
		return ret;
	}
	
	SrsASrsAmf0EcmaArray* arr = dynamic_cast<SrsASrsAmf0EcmaArray*>(any);
	if (!arr) {
		ret = ERROR_RTMP_AMF0_DECODE;
		srs_error("decode metadata array failed. ret=%d", ret);
		srs_freep(any);
		return ret;
	}
	
	for (int i = 0; i < arr->size(); i++) {
		metadata->set(arr->key_at(i), arr->value_at(i));
	}
	arr->clear();
	srs_info("decode metadata array success");
	
	return ret;
}

int SrsOnMetaDataPacket::get_perfer_cid()
{
	return RTMP_CID_OverConnection2;
}

int SrsOnMetaDataPacket::get_message_type()
{
	return RTMP_MSG_AMF0DataMessage;
}

int SrsOnMetaDataPacket::get_size()
{
	return srs_amf0_get_string_size(name) + srs_amf0_get_object_size(metadata);
}

int SrsOnMetaDataPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if ((ret = srs_amf0_write_string(stream, name)) != ERROR_SUCCESS) {
		srs_error("encode name failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode name success.");
	
	if ((ret = srs_amf0_write_object(stream, metadata)) != ERROR_SUCCESS) {
		srs_error("encode metadata failed. ret=%d", ret);
		return ret;
	}
	srs_verbose("encode metadata success.");
	
	srs_info("encode onMetaData packet success.");
	return ret;
}

SrsSetWindowAckSizePacket::SrsSetWindowAckSizePacket()
{
	ackowledgement_window_size = 0;
}

SrsSetWindowAckSizePacket::~SrsSetWindowAckSizePacket()
{
}

int SrsSetWindowAckSizePacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if (!stream->require(4)) {
		ret = ERROR_RTMP_MESSAGE_DECODE;
		srs_error("decode ack window size failed. ret=%d", ret);
		return ret;
	}
	
	ackowledgement_window_size = stream->read_4bytes();
	srs_info("decode ack window size success");
	
	return ret;
}

int SrsSetWindowAckSizePacket::get_perfer_cid()
{
	return RTMP_CID_ProtocolControl;
}

int SrsSetWindowAckSizePacket::get_message_type()
{
	return RTMP_MSG_WindowAcknowledgementSize;
}

int SrsSetWindowAckSizePacket::get_size()
{
	return 4;
}

int SrsSetWindowAckSizePacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if (!stream->require(4)) {
		ret = ERROR_RTMP_MESSAGE_ENCODE;
		srs_error("encode ack size packet failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_4bytes(ackowledgement_window_size);
	
	srs_verbose("encode ack size packet "
		"success. ack_size=%d", ackowledgement_window_size);
	
	return ret;
}

SrsAcknowledgementPacket::SrsAcknowledgementPacket()
{
	sequence_number = 0;
}

SrsAcknowledgementPacket::~SrsAcknowledgementPacket()
{
}

int SrsAcknowledgementPacket::get_perfer_cid()
{
	return RTMP_CID_ProtocolControl;
}

int SrsAcknowledgementPacket::get_message_type()
{
	return RTMP_MSG_Acknowledgement;
}

int SrsAcknowledgementPacket::get_size()
{
	return 4;
}

int SrsAcknowledgementPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if (!stream->require(4)) {
		ret = ERROR_RTMP_MESSAGE_ENCODE;
		srs_error("encode acknowledgement packet failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_4bytes(sequence_number);
	
	srs_verbose("encode acknowledgement packet "
		"success. sequence_number=%d", sequence_number);
	
	return ret;
}

SrsSetChunkSizePacket::SrsSetChunkSizePacket()
{
	chunk_size = RTMP_DEFAULT_CHUNK_SIZE;
}

SrsSetChunkSizePacket::~SrsSetChunkSizePacket()
{
}

int SrsSetChunkSizePacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if (!stream->require(4)) {
		ret = ERROR_RTMP_MESSAGE_DECODE;
		srs_error("decode chunk size failed. ret=%d", ret);
		return ret;
	}
	
	chunk_size = stream->read_4bytes();
	srs_info("decode chunk size success. chunk_size=%d", chunk_size);
	
	if (chunk_size < RTMP_MIN_CHUNK_SIZE) {
		ret = ERROR_RTMP_CHUNK_SIZE;
		srs_error("invalid chunk size. min=%d, actual=%d, ret=%d", 
			ERROR_RTMP_CHUNK_SIZE, chunk_size, ret);
		return ret;
	}
	
	return ret;
}

int SrsSetChunkSizePacket::get_perfer_cid()
{
	return RTMP_CID_ProtocolControl;
}

int SrsSetChunkSizePacket::get_message_type()
{
	return RTMP_MSG_SetChunkSize;
}

int SrsSetChunkSizePacket::get_size()
{
	return 4;
}

int SrsSetChunkSizePacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if (!stream->require(4)) {
		ret = ERROR_RTMP_MESSAGE_ENCODE;
		srs_error("encode chunk packet failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_4bytes(chunk_size);
	
	srs_verbose("encode chunk packet success. ack_size=%d", chunk_size);
	
	return ret;
}

SrsSetPeerBandwidthPacket::SrsSetPeerBandwidthPacket()
{
	bandwidth = 0;
	type = 2;
}

SrsSetPeerBandwidthPacket::~SrsSetPeerBandwidthPacket()
{
}

int SrsSetPeerBandwidthPacket::get_perfer_cid()
{
	return RTMP_CID_ProtocolControl;
}

int SrsSetPeerBandwidthPacket::get_message_type()
{
	return RTMP_MSG_SetPeerBandwidth;
}

int SrsSetPeerBandwidthPacket::get_size()
{
	return 5;
}

int SrsSetPeerBandwidthPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if (!stream->require(5)) {
		ret = ERROR_RTMP_MESSAGE_ENCODE;
		srs_error("encode set bandwidth packet failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_4bytes(bandwidth);
	stream->write_1bytes(type);
	
	srs_verbose("encode set bandwidth packet "
		"success. bandwidth=%d, type=%d", bandwidth, type);
	
	return ret;
}

SrsUserControlPacket::SrsUserControlPacket()
{
	event_type = 0;
	event_data = 0;
	extra_data = 0;
}

SrsUserControlPacket::~SrsUserControlPacket()
{
}

int SrsUserControlPacket::decode(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if (!stream->require(6)) {
		ret = ERROR_RTMP_MESSAGE_DECODE;
		srs_error("decode user control failed. ret=%d", ret);
		return ret;
	}
	
	event_type = stream->read_2bytes();
	event_data = stream->read_4bytes();
	
	if (event_type == SrcPCUCSetBufferLength) {
		if (!stream->require(4)) {
			ret = ERROR_RTMP_MESSAGE_ENCODE;
			srs_error("decode user control packet failed. ret=%d", ret);
			return ret;
		}
		extra_data = stream->read_4bytes();
	}
	
	srs_info("decode user control success. "
		"event_type=%d, event_data=%d, extra_data=%d", 
		event_type, event_data, extra_data);
	
	return ret;
}

int SrsUserControlPacket::get_perfer_cid()
{
	return RTMP_CID_ProtocolControl;
}

int SrsUserControlPacket::get_message_type()
{
	return RTMP_MSG_UserControlMessage;
}

int SrsUserControlPacket::get_size()
{
	if (event_type == SrcPCUCSetBufferLength) {
		return 2 + 4 + 4;
	} else {
		return 2 + 4;
	}
}

int SrsUserControlPacket::encode_packet(SrsStream* stream)
{
	int ret = ERROR_SUCCESS;
	
	if (!stream->require(get_size())) {
		ret = ERROR_RTMP_MESSAGE_ENCODE;
		srs_error("encode user control packet failed. ret=%d", ret);
		return ret;
	}
	
	stream->write_2bytes(event_type);
	stream->write_4bytes(event_data);

	// when event type is set buffer length,
	// read the extra buffer length.
	if (event_type == SrcPCUCSetBufferLength) {
		stream->write_2bytes(extra_data);
		srs_verbose("user control message, buffer_length=%d", extra_data);
	}
	
	srs_verbose("encode user control packet success. "
		"event_type=%d, event_data=%d", event_type, event_data);
	
	return ret;
}

SrsRtmpClient::SrsRtmpClient(st_netfd_t _stfd)
{
	stfd = _stfd;
	protocol = new SrsProtocol(stfd);
}

SrsRtmpClient::~SrsRtmpClient()
{
	srs_freep(protocol);
}

void SrsRtmpClient::set_recv_timeout(int64_t timeout_us)
{
	protocol->set_recv_timeout(timeout_us);
}

void SrsRtmpClient::set_send_timeout(int64_t timeout_us)
{
	protocol->set_send_timeout(timeout_us);
}

int64_t SrsRtmpClient::get_recv_bytes()
{
	return protocol->get_recv_bytes();
}

int64_t SrsRtmpClient::get_send_bytes()
{
	return protocol->get_send_bytes();
}

int SrsRtmpClient::get_recv_kbps()
{
	return protocol->get_recv_kbps();
}

int SrsRtmpClient::get_send_kbps()
{
	return protocol->get_send_kbps();
}

int SrsRtmpClient::recv_message(SrsCommonMessage** pmsg)
{
	return protocol->recv_message(pmsg);
}

int SrsRtmpClient::send_message(ISrsMessage* msg)
{
	return protocol->send_message(msg);
}

int SrsRtmpClient::handshake()
{
	int ret = ERROR_SUCCESS;
	
    SrsSocket skt(stfd);
    
    SrsComplexHandshake complex_hs;
    SrsSimpleHandshake simple_hs;
    if ((ret = simple_hs.handshake_with_server(skt, complex_hs)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsRtmpClient::connect_app(std::string app, std::string tc_url)
{
	int ret = ERROR_SUCCESS;
	
	// Connect(vhost, app)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsConnectAppPacket* pkt = new SrsConnectAppPacket();
		msg->set_packet(pkt, 0);
		
		pkt->command_object = new SrsAmf0Object();
		pkt->command_object->set("app", new SrsAmf0String(app.c_str()));
		pkt->command_object->set("swfUrl", new SrsAmf0String());
		pkt->command_object->set("tcUrl", new SrsAmf0String(tc_url.c_str()));
		pkt->command_object->set("fpad", new SrsAmf0Boolean(false));
		pkt->command_object->set("capabilities", new SrsAmf0Number(239));
		pkt->command_object->set("audioCodecs", new SrsAmf0Number(3575));
		pkt->command_object->set("videoCodecs", new SrsAmf0Number(252));
		pkt->command_object->set("videoFunction", new SrsAmf0Number(1));
		pkt->command_object->set("pageUrl", new SrsAmf0String());
		pkt->command_object->set("objectEncoding", new SrsAmf0Number(0));
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			return ret;
		}
	}
	
	// Set Window Acknowledgement size(2500000)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsSetWindowAckSizePacket* pkt = new SrsSetWindowAckSizePacket();
	
		pkt->ackowledgement_window_size = 2500000;
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			return ret;
		}
	}
	
	// expect connect _result
	SrsCommonMessage* msg = NULL;
	SrsConnectAppResPacket* pkt = NULL;
	if ((ret = srs_rtmp_expect_message<SrsConnectAppResPacket>(protocol, &msg, &pkt)) != ERROR_SUCCESS) {
		srs_error("expect connect app response message failed. ret=%d", ret);
		return ret;
	}
	SrsAutoFree(SrsCommonMessage, msg, false);
	srs_info("get connect app response message");
	
    return ret;
}

int SrsRtmpClient::create_stream(int& stream_id)
{
	int ret = ERROR_SUCCESS;
	
	// CreateStream
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsCreateStreamPacket* pkt = new SrsCreateStreamPacket();
	
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			return ret;
		}
	}

	// CreateStream _result.
	if (true) {
		SrsCommonMessage* msg = NULL;
		SrsCreateStreamResPacket* pkt = NULL;
		if ((ret = srs_rtmp_expect_message<SrsCreateStreamResPacket>(protocol, &msg, &pkt)) != ERROR_SUCCESS) {
			srs_error("expect create stream response message failed. ret=%d", ret);
			return ret;
		}
		SrsAutoFree(SrsCommonMessage, msg, false);
		srs_info("get create stream response message");

		stream_id = (int)pkt->stream_id;
	}
	
	return ret;
}

int SrsRtmpClient::play(std::string stream, int stream_id)
{
	int ret = ERROR_SUCCESS;

	// Play(stream)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsPlayPacket* pkt = new SrsPlayPacket();
	
		pkt->stream_name = stream;
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send play stream failed. "
				"stream=%s, stream_id=%d, ret=%d", 
				stream.c_str(), stream_id, ret);
			return ret;
		}
	}
	
	// SetBufferLength(1000ms)
	int buffer_length_ms = 1000;
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsUserControlPacket* pkt = new SrsUserControlPacket();
	
		pkt->event_type = SrcPCUCSetBufferLength;
		pkt->event_data = stream_id;
		pkt->extra_data = buffer_length_ms;
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send set buffer length failed. "
				"stream=%s, stream_id=%d, bufferLength=%d, ret=%d", 
				stream.c_str(), stream_id, buffer_length_ms, ret);
			return ret;
		}
	}
	
	return ret;
}

int SrsRtmpClient::publish(std::string stream, int stream_id)
{
	int ret = ERROR_SUCCESS;

	// publish(stream)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsPublishPacket* pkt = new SrsPublishPacket();
	
		pkt->stream_name = stream;
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send publish message failed. "
				"stream=%s, stream_id=%d, ret=%d", 
				stream.c_str(), stream_id, ret);
			return ret;
		}
	}
	
	return ret;
}

