/*
The MIT License (MIT)

Copyright (c) 2013-2015 winlin

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

#include <htl_stdinc.hpp>
#include <htl_app_srs_hijack.hpp>

#include <htl_os_st.hpp>
#include <htl_core_error.hpp>
#include <htl_app_rtmp_protocol.hpp>

#ifndef SRS_HIJACK_IO
    #error "must hijack the srs-librtmp"
#endif

StSocket* srs_hijack_get(srs_rtmp_t rtmp)
{
    srs_hijack_io_t ctx = srs_hijack_io_get(rtmp);
    return (StSocket*)ctx;
}

srs_hijack_io_t srs_hijack_io_create()
{
    return new StSocket();
}

void srs_hijack_io_destroy(srs_hijack_io_t ctx)
{
    StSocket* skt = (StSocket*)ctx;
    delete skt;
}

int srs_hijack_io_create_socket(srs_hijack_io_t /*ctx*/, srs_rtmp_t /*owner*/)
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

int srs_hijack_io_set_recv_timeout(srs_hijack_io_t /*ctx*/, int64_t /*tm*/)
{
    return 0;
}

int64_t srs_hijack_io_get_recv_timeout(srs_hijack_io_t /*ctx*/)
{
    return ST_UTIME_NO_TIMEOUT;
}

int64_t srs_hijack_io_get_recv_bytes(srs_hijack_io_t /*ctx*/)
{
    return 0;
}

int srs_hijack_io_set_send_timeout(srs_hijack_io_t /*ctx*/, int64_t /*tm*/)
{
    return 0;
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

int srs_hijack_io_is_never_timeout(srs_hijack_io_t /*ctx*/, int64_t /*tm*/)
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
