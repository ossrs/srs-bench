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

#include <inttypes.h>
#include <stdlib.h>

#include <string>
#include <sstream>
#include <vector>
using namespace std;

#include <htl_core_error.hpp>
#include <htl_core_log.hpp>
#include <htl_app_http_client.hpp>

#include <htl_app_m3u8_parser.hpp>

class String
{
private:
    string value;
public:
    String(string str = ""){
        value = str;
    }
public:
    String& set_str(string str){
        value = str;
        return *this;
    }
    int length(){
        return (int)value.length();
    }
    bool startswith(string key, String* left = NULL){
        size_t pos = value.find(key);
        
        if(pos == 0 && left != NULL){
            left->set_str(value.substr(pos + key.length()));
            left->strip();
        }
        
        return pos == 0;
    }
    bool endswith(string key, String* left = NULL){
        size_t pos = value.rfind(key);
        
        if(pos == value.length() - key.length() && left != NULL){
            left->set_str(value.substr(pos));
            left->strip();
        }
        
        return pos == value.length() - key.length();
    }
    String& strip(){
        if (value.empty()) {
            return *this;
        }
        
        // macro to test whether char is a space.
        #define __IS_SPACE(ch) (ch == '\n' || ch == '\r' || ch == ' ')
        
        // find the start and end which is not space.
        char* bytes = (char*)value.data();
        
        // find the end not space
        char* end = NULL;
        for (end = bytes + value.length() - 1; end > bytes && __IS_SPACE(end[0]); end--) {
        }

        // find the start not space
        char* start = NULL;
        for (start = bytes; start < end && __IS_SPACE(start[0]); start++) {
        }
        
        // get the data to trim.
        value = value.substr(start - bytes, end - start + 1);
        
        return *this;
    }
    string getline(){
        size_t pos = string::npos;
        
        if((pos = value.find("\n")) != string::npos){
            return value.substr(0, pos);
        }
        
        return value;
    }
    String& remove(int size){
        if(size >= (int)value.length()){
            value = "";
            return *this;
        }
        
        value = value.substr(size);
        return *this;
    }
    const char* c_str(){
        return value.c_str();
    }
};

HlsM3u8Parser::HlsM3u8Parser(){
}

HlsM3u8Parser::~HlsM3u8Parser(){
}

int HlsM3u8Parser::ParseM3u8Data(HttpUrl* url, string m3u8, vector<M3u8TS>& ts_objects, int& target_duration, string& variant){
    int ret = ERROR_SUCCESS;
    
    String data(m3u8);
    
    // http://tools.ietf.org/html/draft-pantos-http-live-streaming-08#section-3.3.1
    // An Extended M3U file is distinguished from a basic M3U file by its
    // first line, which MUST be the tag #EXTM3U.
    if(!data.startswith("#EXTM3U")){
        ret = ERROR_HLS_INVALID;
        Error("invalid hls, #EXTM3U not found. ret=%d", ret);
        return ret;
    }
    
    String value;
    
    M3u8TS ts_object;
    while(data.length() > 0){
        String line;
        data.remove(line.set_str(data.strip().getline()).strip().length()).strip();

        // http://tools.ietf.org/html/draft-pantos-http-live-streaming-08#section-3.4.2
        // #EXT-X-TARGETDURATION:<s>
        // where s is an integer indicating the target duration in seconds.
        if(line.startswith("#EXT-X-TARGETDURATION:", &value)){
            target_duration = atoi(value.c_str());
            ts_object.duration = target_duration;
            continue;
        }
        
        // #EXT-X-STREAM-INF:BANDWIDTH=3000000
        // http://192.168.13.108:28080/gh.b0.upaiyun.com/app01/stream01.m3u8
        if (line.startswith("#EXT-X-STREAM-INF:", &value)) {
            variant = data.strip().getline();
            return ret;
        }
        
        // http://tools.ietf.org/html/draft-pantos-http-live-streaming-08#section-3.3.2
        // #EXTINF:<duration>,<title>
        // "duration" is an integer or floating-point number in decimal
        // positional notation that specifies the duration of the media segment
        // in seconds.  Durations that are reported as integers SHOULD be
        // rounded to the nearest integer.  Durations MUST be integers if the
        // protocol version of the Playlist file is less than 3.  The remainder
        // of the line following the comma is an optional human-readable
        // informative title of the media segment.
        // ignore others util EXTINF
        if(line.startswith("#EXTINF:", &value)){
            ts_object.duration = atof(value.c_str());
            continue;
        }
        
        if(!line.startswith("#")){
            ts_object.ts_url = url->Resolve(line.c_str());
            ts_objects.push_back(ts_object);
            continue;
        }
    }
    
    return ret;
}

