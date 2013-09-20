// system
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>

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

int StFarm::Initialize(){
    int ret = ERROR_SUCCESS;
    
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
    
    st_thread_exit(NULL);
    
    return ret;
}

void* StFarm::st_thread_function(void* args){
    StTask* task = (StTask*)args;
    
    context->SetId(task->GetId());
    
    int ret = task->Process();
    
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
    
    *nread = st_read(sock_nfd, (void*)buf, size, ST_UTIME_NO_TIMEOUT);
    
    if(*nread <= 0){
        if(errno == ETIME){
            errno = EAGAIN;
        }
        
        ret = ERROR_READ;
        status = SocketDisconnected;
    }
        
    return ret;
}

int StSocket::Write(const void* buf, size_t size, ssize_t* nwrite){
    int ret = ERROR_SUCCESS;
    
    *nwrite = st_write(sock_nfd, (void*)buf, size, ST_UTIME_NO_TIMEOUT);
    
    if(*nwrite <= 0){
        if(errno == ETIME){
            errno = EAGAIN;
        }
        
        ret = ERROR_SEND;
        status = SocketDisconnected;
    }
        
    return ret;
}

int StSocket::Close(){
    int ret = ERROR_SUCCESS;
    
    if(sock_nfd == NULL){
        return ret;
    }
    
    if(st_netfd_close(sock_nfd) != 0){
        ret = ERROR_CLOSE;
    }
    
    sock_nfd = NULL;
    status = SocketDisconnected;
    
    return ret;
}

void StUtility::InitRandom(){
    timeval now;
    
    if(gettimeofday(&now, NULL) == -1){
        srand(0);
        return;
    }
    
    srand(now.tv_sec * 1000000 + now.tv_usec);
}

st_utime_t StUtility::BuildRandomMTime(double sleep_seconds, double default_seconds){
    if(sleep_seconds <= 0){
        return default_seconds * 1000;
    }
    
    st_utime_t sleep_ms = (int)(sleep_seconds * 1000 * 0.8) + rand() % (int)(sleep_seconds * 1000 * 0.4);
    
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
