// system
#include <unistd.h>
#include <errno.h>

// socket
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// c lib
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// c++ lib
#include <string>
#include <sstream>
using namespace std;

// 3rdparty lib
#include <http_parser.h>

// project lib
#include <htl_core_log.hpp>
#include <htl_core_error.hpp>
#include <htl_os_st.hpp>
#include <htl_app_http_load.hpp>

StHttpTask::StHttpTask(){
}

StHttpTask::~StHttpTask(){
}

int StHttpTask::Process(){
    return ERROR_SUCCESS;
}

int on_message_begin(http_parser* _) {
  (void)_;
  printf("\n***MESSAGE BEGIN***\n\n");
  return 0;
}

int on_headers_complete(http_parser* _) {
  (void)_;
  printf("\n***HEADERS COMPLETE***\n\n");
  return 0;
}

int on_message_complete(http_parser* _) {
  (void)_;
  printf("\n***MESSAGE COMPLETE***\n\n");
  return 0;
}

int on_url(http_parser* _, const char* at, size_t length) {
  (void)_;
  printf("Url: %.*s\n", (int)length, at);
  return 0;
}

int on_header_field(http_parser* _, const char* at, size_t length) {
  (void)_;
  printf("Header field: %.*s\n", (int)length, at);
  return 0;
}

int on_header_value(http_parser* _, const char* at, size_t length) {
  (void)_;
  printf("Header value: %.*s\n", (int)length, at);
  return 0;
}

int on_body(http_parser* _, const char* at, size_t length) {
  (void)_;
  printf("Body: %.*s\n", (int)length, at);
  return 0;
}

int main(int argc, char** argv){
    printf("argc=%d, argv[0]=%s\n", argc, argv[0]);
    
    string url_str;
    url_str = "http://192.168.2.111:3080/hls/hls.ts";
    url_str = "http://192.168.2.111:3080/hls/segm130813144315787-522881.ts";
    
    StFarm farm;
    for(int i = 0; i < 1; i++){
    }
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        Error("create socket error.");
        return ERROR_SOCKET;
    }
    Info("[%d] create socket success", sock);
    
    int reuse_socket = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_socket, sizeof(int)) == -1){
        Error("setsockopt reuse-addr error, errno=%#x", errno);
        return ERROR_SOCKET;
    }
    
    int port = 3080;
    string ip = "192.168.2.111";
    string path = "/hls/segm130813144315787-522881.ts";
    
    // connect to server
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
    if(connect(sock, (const struct sockaddr*)&addr, sizeof(sockaddr_in)) == -1){
        Error("[%d] connect to server(%s:%d) error.", sock, ip.c_str(), port);
        return ERROR_CONNECT;
    }
    Info("[%d] connec to server %s at port %d success", sock, ip.c_str(), port);
    
    // send GET request to read content
    // GET %s HTTP/1.1\r\nHost: %s\r\n\r\n
    stringstream ss;
    ss << "GET " << path << " "
        << "HTTP/1.1\r\n"
        << "Host: " << ip << "\r\n" 
        << "Connection: Keep-Alive" << "\r\n"
        << "\r\n";
        
    string data = ss.str();
    int nsend = 0;
    if((nsend = write(sock, data.c_str(), (int)data.length())) != (int)data.length()){
        int ret = ERROR_SEND;
        Error("[%d] send to server %s error, send=%d, total=%d, ret=%d", sock, ip.c_str(), nsend, (int)data.length(), ret);
        return ret;
    }
    Info("[%d] send request %s success", sock, path.c_str());

    http_parser_type file_type = HTTP_RESPONSE;

    http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    settings.on_message_begin = on_message_begin;
    settings.on_url = on_url;
    settings.on_header_field = on_header_field;
    settings.on_header_value = on_header_value;
    settings.on_headers_complete = on_headers_complete;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;
    
    http_parser parser;
    http_parser_init(&parser, file_type);
    parser.data = (void*)sock; // can set to obj ptr.
    
    for(;;){
        char buf[230];
        memset(buf, 0, sizeof(buf));
        
        int nread = 0;
        if((nread = read(sock, buf, sizeof(buf))) <= 0){
            int ret = ERROR_READ;
            Error("[%d] read from server failed, read_size=%d, ret=%d", sock, nread, ret);
            return ret;
        }
        
        size_t nparsed = http_parser_execute(&parser, &settings, buf, nread);
        Info("[%d] read_size=%d, nparsed=%d", sock, (int)nread, (int)nparsed);
    }
    
    return 0;
}
