#pragma once 

#include "http_message.h"
#include <string>
#include <cstring>
#include <vector>
#include <cassert>

using namespace std; 

class HttpParser {
public:
    enum class ParseResult {
        SUCCESS = 0,
        AGAIN,
        ERROR
    };

    using Headers = std::vector<std::pair<std::string, std::string>>;

public:
    HttpParser(); 
    ~HttpParser() = default;
    ParseResult parse(const char* data, const size_t len, size_t& parsed_size);
    std::string encode() const ; 
    
    const char* getMethod() const ;
    const char* getUri() const ;
    const char* getVersion() const ;
    const Headers& getHeaders() const ; 
    const char* getBody() const ; 
    size_t getBodySize() const;
    bool getKeepAlive() const; 

    bool parsingCompletion(); 

private:
    enum class ParsePhase {
        REQUEST_LINE = 0,
        HEADER, 
        BODY, 
        DONE
    };

    enum class ParseRequestLineState {
        BEFORE_METHOD = 0,
        METHOD, 
        BEFORE_URI, 
        URI,
        BEFORE_VERSION,
        VERSION, 
        LF,
    };

    enum class ParseHeaderState {
        BEFORE_KEY = 0,
        KEY,
        SPACE,
        BEFORE_VALUE,
        VALUE, 
        LF, 
        END_CR, 
        END_LF, 
    }; 

private:
    void reset(); 
    ParseResult parseRequestLine(); 
    ParseResult parseHeader();
    ParseResult parseBody(); 
    bool checkMethod(const char* str, size_t len); 
    bool checkUri(const char* str, size_t len); 
    bool checkVersion(const char* str, size_t len);
    bool checkHeader(const char* name, size_t n_len, const char* value, size_t v_len); 

private:
    static const int HEADER_NAME_MAX = 128; 

    ParsePhase parse_phase; 
    ParseRequestLineState parse_rl_state;
    ParseHeaderState parse_hd_state;  

    std::string method;
    std::string uri;
    std::string version;
    char name_buf[HEADER_NAME_MAX]; 
    size_t name_buf_pos;  
    Headers headers; 
    std::vector<char> body;
    size_t content_length;   // content-length头部值, 若存在. 
    size_t transfer_length;  // 消息体实际长度(考虑chunked 分块编码的情况)
    size_t header_offset;    // 当前已解析的头部偏移量; 
    size_t body_offset;      // 当前已解析的请求体偏移量. 
    bool has_connection;     // 是否具有"Connection"头. 
    bool has_keep_alive;     // 是否具有"Keep-Alive"头. 
    bool keep_alive;         // 是否为长连接
    bool chunked;
    const char* pos;         // 记录当前`parse()`调用中的当前解析位置. 
    const char* end;         // 记录当前`parse()`调用中输入data的边界. 
};