#include <htl_stdinc.hpp>
#include <htl_app_srs_hijack.hpp>

#include <htl_os_st.hpp>
#include <htl_core_error.hpp>
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
