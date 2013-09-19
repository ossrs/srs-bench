#include<unistd.h>

#include <stdio.h>

#include <string>
#include <sstream>
using namespace std;

#include <http_parser.h>

int main(int argc, char** argv){
    printf("argc=%d, argv[0]=%s\n", argc, argv[0]);
    
    string url_str;
    url_str = "http://192.168.2.111:3080/hls/hls.ts";
    url_str = "http://192.168.2.111:3080/hls/segm130813144315787-522881.ts";
    
    return 0;
}
