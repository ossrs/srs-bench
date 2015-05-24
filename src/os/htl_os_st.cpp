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

// system
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <inttypes.h>

// socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// dns
#include <netdb.h>

#include <string>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>

#include <htl_os_st.hpp>

StStatistic::StStatistic(){
    starttime = StUtility::GetCurrentTime();
    task_duration = 0;
    threads = alive = 0;
    nread = nwrite = 0;
    tasks = err_tasks = sub_tasks = err_sub_tasks = 0;
}

StStatistic::~StStatistic(){
}

void StStatistic::OnRead(int /*tid*/, ssize_t nread_bytes){
    this->nread += nread_bytes;
}

void StStatistic::OnWrite(int /*tid*/, ssize_t nwrite_bytes){
    this->nwrite += nwrite_bytes;
}

void StStatistic::OnThreadRun(int /*tid*/){
    threads++;
}

void StStatistic::OnThreadQuit(int /*tid*/){
    threads--;
}

void StStatistic::OnTaskStart(int /*tid*/, std::string /*task_url*/){
    alive++;
    tasks++;
}

void StStatistic::OnTaskError(int /*tid*/, int duration_seconds){
    alive--;
    err_tasks++;
    this->task_duration += duration_seconds * 1000;
}

void StStatistic::OnTaskEnd(int /*tid*/, int duration_seconds){
    alive--;
    this->task_duration += duration_seconds * 1000;
}

void StStatistic::OnSubTaskStart(int /*tid*/, std::string /*sub_task_url*/){
    sub_tasks++;
}

void StStatistic::OnSubTaskError(int /*tid*/, int duration_seconds){
    err_sub_tasks++;
    this->task_duration += duration_seconds * 1000;
}

void StStatistic::OnSubTaskEnd(int /*tid*/, int duration_seconds){
    this->task_duration += duration_seconds * 1000;
}

void StStatistic::DoReport(double sleep_ms){
    for(;;){
        int64_t duration = StUtility::GetCurrentTime() - starttime;
        double read_mbps = 0, write_mbps = 0;
        
        if(duration > 0){
            read_mbps = nread * 8.0 / duration / 1000;
            write_mbps = nwrite * 8.0 / duration / 1000;
        }
        
        double avarage_duration = task_duration/1000.0;
        if(tasks > 0){
            avarage_duration /= tasks;
        }
        LReport("[report] [%d] threads:%d alive:%d duration:%.0f tduration:%.0f nread:%.2f nwrite:%.2f "
            "tasks:%"PRId64" etasks:%"PRId64" stasks:%"PRId64" estasks:%"PRId64,
            getpid(), threads, alive, duration/1000.0, avarage_duration, read_mbps, write_mbps, 
            tasks, err_tasks, sub_tasks, err_sub_tasks);
        
        st_usleep((st_utime_t)(sleep_ms * 1000));
    }
}

StStatistic* statistic = new StStatistic();

StTask::StTask(){
    static int _id = 0;
    id = ++_id;
}

StTask::~StTask(){
}

int StTask::GetId(){
    return id;
}

StFarm::StFarm(){
}

StFarm::~StFarm(){
}

int StFarm::Initialize(double report){
    int ret = ERROR_SUCCESS;
    
    report_seconds = report;
    
    // use linux epoll.
    if(st_set_eventsys(ST_EVENTSYS_ALT) == -1){
        ret = ERROR_ST_INITIALIZE;
        Error("st_set_eventsys use linux epoll failed. ret=%d", ret);
        return ret;
    }
    
    if(st_init() != 0){
        ret = ERROR_ST_INITIALIZE;
        Error("st_init failed. ret=%d", ret);
        return ret;
    }
    
    StUtility::InitRandom();
    
    return ret;
}

int StFarm::Spawn(StTask* task){
    int ret = ERROR_SUCCESS;
    
    if(st_thread_create(st_thread_function, task, 0, 0) == NULL){
        ret = ERROR_ST_THREAD_CREATE;
        Error("crate st_thread failed, ret=%d", ret);
        return ret;
    }
    
    Trace("create thread for task #%d success", task->GetId());
    
    return ret;
}

int StFarm::WaitAll(){
    int ret = ERROR_SUCCESS;
    
    // main thread turn to a report therad.
    statistic->DoReport(report_seconds * 1000);
    
    st_thread_exit(NULL);
    
    return ret;
}

void* StFarm::st_thread_function(void* args){
    StTask* task = (StTask*)args;
    
    context->SetId(task->GetId());
    
    statistic->OnThreadRun(task->GetId());
    
    int ret = task->Process();
    
    statistic->OnThreadQuit(task->GetId());
    
    if(ret != ERROR_SUCCESS){
        Warn("st task terminate with ret=%d", ret);
    }
    else{
        Trace("st task terminate with ret=%d", ret);
    }

    delete task;
    
    return NULL;
}

StSocket::StSocket(){
    sock_nfd = NULL;
    status = SocketInit;
}

StSocket::~StSocket(){
    Close();
}

st_netfd_t StSocket::GetStfd(){
	return sock_nfd;
}

SocketStatus StSocket::Status(){
    return status;
}

int StSocket::Connect(const char* ip, int port){
    int ret = ERROR_SUCCESS;
    
    Close();
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        ret = ERROR_SOCKET;
        Error("create socket error. ret=%d", ret);
        return ret;
    }
    
    int reuse_socket = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_socket, sizeof(int)) == -1){
        ret = ERROR_SOCKET;
        Error("setsockopt reuse-addr error. ret=%d", ret);
        return ret;
    }
    
    int keepalive_socket = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepalive_socket, sizeof(int)) == -1){
        ret = ERROR_SOCKET;
        Error("setsockopt keep-alive error. ret=%d", ret);
        return ret;
    }

    sock_nfd = st_netfd_open_socket(sock);
    if(sock_nfd == NULL){
        ret = ERROR_OPEN_SOCKET;
        Error("st_netfd_open_socket failed. ret=%d", ret);
        return ret;
    }
    Info("create socket(%d) success", sock);
        
    
    // connect to server
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    
    if(st_connect(sock_nfd, (const struct sockaddr*)&addr, sizeof(sockaddr_in), ST_UTIME_NO_TIMEOUT) == -1){
        ret = ERROR_CONNECT;
        Error("connect to server(%s:%d) error. ret=%d", ip, port, ret);
        return ret;
    }
    Info("connec to server %s at port %d success", ip, port);
    
    status = SocketConnected;
    
    return ret;
}

int StSocket::Read(const void* buf, size_t size, ssize_t* nread){
    int ret = ERROR_SUCCESS;
    
    ssize_t got = st_read(sock_nfd, (void*)buf, size, ST_UTIME_NO_TIMEOUT);
    
    // On success a non-negative integer indicating the number of bytes actually read is returned 
    // (a value of 0 means the network connection is closed or end of file is reached).
    if(got <= 0){
        if(got == 0){
            errno = ECONNRESET;
        }
        
        ret = ERROR_READ;
        status = SocketDisconnected;
    }
    
    if(got > 0){
        statistic->OnRead(context->GetId(), got);
    }
    
    if (nread) {
        *nread = got;
    }
        
    return ret;
}

int StSocket::Readv(const iovec *iov, int iov_size, ssize_t* nread){
    int ret = ERROR_SUCCESS;
    
    ssize_t got = st_readv(sock_nfd, iov, iov_size, ST_UTIME_NO_TIMEOUT);
    
    // On success a non-negative integer indicating the number of bytes actually read is returned 
    // (a value of 0 means the network connection is closed or end of file is reached).
    if(got <= 0){
        if(got == 0){
            errno = ECONNRESET;
        }
        
        ret = ERROR_READ;
        status = SocketDisconnected;
    }
    
    if(got > 0){
        statistic->OnRead(context->GetId(), got);
    }
    
    if (nread) {
        *nread = got;
    }
        
    return ret;
}

int StSocket::ReadFully(const void* buf, size_t size, ssize_t* nread){
    int ret = ERROR_SUCCESS;
    
    ssize_t got = st_read_fully(sock_nfd, (void*)buf, size, ST_UTIME_NO_TIMEOUT);
    
    // On success a non-negative integer indicating the number of bytes actually read is returned 
    // (a value less than nbyte means the network connection is closed or end of file is reached)
    if(got != (ssize_t)size){
        if(got >= 0){
            errno = ECONNRESET;
        }
        
        ret = ERROR_READ;
        status = SocketDisconnected;
    }
    
    if(got > 0){
        statistic->OnRead(context->GetId(), got);
    }
    
    if (nread) {
        *nread = got;
    }
        
    return ret;
}

int StSocket::Write(const void* buf, size_t size, ssize_t* nwrite){
    int ret = ERROR_SUCCESS;
    
    ssize_t writen = st_write(sock_nfd, (void*)buf, size, ST_UTIME_NO_TIMEOUT);
    
    if(writen <= 0){
        ret = ERROR_SEND;
        status = SocketDisconnected;
    }
    
    if(writen > 0){
        statistic->OnWrite(context->GetId(), writen);
    }
    
    if (nwrite) {
        *nwrite = writen;
    }
        
    return ret;
}

int StSocket::Writev(const iovec *iov, int iov_size, ssize_t* nwrite){
    int ret = ERROR_SUCCESS;
    
    ssize_t writen = st_writev(sock_nfd, iov, iov_size, ST_UTIME_NO_TIMEOUT);
    
    if(writen <= 0){
        ret = ERROR_SEND;
        status = SocketDisconnected;
    }
    
    if(writen > 0){
        statistic->OnWrite(context->GetId(), writen);
    }
    
    if (nwrite) {
        *nwrite = writen;
    }
        
    return ret;
}

int StSocket::Close(){
    int ret = ERROR_SUCCESS;
    
    if(sock_nfd == NULL){
        return ret;
    }
    
    int fd = st_netfd_fileno(sock_nfd);
    if(st_netfd_close(sock_nfd) != 0){
        ret = ERROR_CLOSE;
    }
    
    sock_nfd = NULL;
    status = SocketDisconnected;
    
    ::close(fd);
    
    return ret;
}

int64_t StUtility::GetCurrentTime(){
    timeval now;
    
    int ret = gettimeofday(&now, NULL);
    
    if(ret == -1){
        Warn("gettimeofday error, ret=%d", ret);
    }

    // we must convert the tv_sec/tv_usec to int64_t.
    return ((int64_t)now.tv_sec)*1000 + ((int64_t)now.tv_usec) / 1000;
}

void StUtility::InitRandom(){
    timeval now;
    
    if(gettimeofday(&now, NULL) == -1){
        srand(0);
        return;
    }
    
    srand(now.tv_sec * 1000000 + now.tv_usec);
}

st_utime_t StUtility::BuildRandomMTime(double sleep_seconds){
    if(sleep_seconds <= 0){
        return 0 * 1000;
    }
    
    // 80% consts value.
    // 40% random value.
    // to get more graceful random time to mocking HLS client.
    st_utime_t sleep_ms = (int)(sleep_seconds * 1000 * 0.7) + rand() % (int)(sleep_seconds * 1000 * 0.4);
    
    return sleep_ms;
}

int StUtility::DnsResolve(string host, string& ip){
    int ret = ERROR_SUCCESS;
    
    if(inet_addr(host.c_str()) != INADDR_NONE){
        ip = host;
        Info("dns resolve %s to %s", host.c_str(), ip.c_str());
        
        return ret;
    }
    
    hostent* answer = gethostbyname(host.c_str());
    if(answer == NULL){
        ret = ERROR_DNS_RESOLVE;
        Error("dns resolve host %s error. ret=%d", host.c_str(), ret);
        return ret;
    }
    
    char ipv4[16];
    memset(ipv4, 0, sizeof(ipv4));
    for(int i = 0; i < answer->h_length; i++){
        inet_ntop(AF_INET, answer->h_addr_list[i], ipv4, sizeof(ipv4));
        Info("dns resolve host %s to %s.", host.c_str(), ipv4);
        break;
    }
    
    ip = ipv4;
    Info("dns resolve %s to %s", host.c_str(), ip.c_str());
    
    return ret;
}

StLogContext::StLogContext(){
}

StLogContext::~StLogContext(){
}

void StLogContext::SetId(int id){
    cache[st_thread_self()] = id;
}

int StLogContext::GetId(){
    return cache[st_thread_self()];
}

