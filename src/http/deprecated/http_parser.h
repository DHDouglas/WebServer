/* 主-从状态机
 * 主状态机: 在各个解析阶段之间转移. 
 * 从状态机: 逐个扫描字符, 根据字符进行转移. 
 */


// 逐个字符扫描, 将字符作为输入事件, 根据当前状态进行处理; 
// 找到明显的"分界符"提取token, 再cmp检验token是否合法. 
// 为了能够续接, 要定义INCOMPLETE状态,
// 当前整体解析状态: SUCCESS, AGAIN, ERROR
// - 各阶段的解析状态: SUCCESS, AGAIN, ERROR. => 标识解析状态.  
// 当前所在的解析阶段: PARSE_REQ_LINE, PARSE_HEADERS, PARSE_CONTENT; 
// - 解析请求行所在的具体状态;
// - 解析请求头所在的具体状态; 
// 已解析得到的各个字段:
// - 起始下标(相较于缓冲区read_post起始地址)
// - 长度. 
// 当前所在的解析位置. 
// 
// TODO: 如何保存解析所得的数据.  
// 1) Parser全局记录一个起始地址const char* text.  
// 2) 使用结构体, 第一项存储指针标记字段起始位置, 第二项存储字段长度[pos_ptr, pos_ptr+len).
// 需要保存读取到的信息, 如果method这些字段使用(指针, 长度)来记录, 就没办法保存以"具体有效值"的形式保存结果, 不便于判断. 
// 例如无法在解析出method后, 判断其
// 例如不便于判断是否存在content-length头部. 

#include <chrono>
#include <string>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <cassert>

using namespace std;

enum class HttpMethod {
    GET = 0, 
    POST,
    HEAD, 
    UNKNOWN
};

enum class ParseResult {
    SUCCESS = 0, 
    AGAIN, 
    ERROR
};

enum class ParsePhase {
    REQUEST_LINE,
    HEADER,
    CONTENT
};

enum class ParseRequestLineState {
    START, 
    METHOD,
    BEFORE_URI,
    URI,
    BEFORE_VERSION, 
    VERSION, 
    CR,
    // LF,
}; 

enum class ParseHeaderState {
    START, 
    KEY,
    COLON,
    SPACE, 
    VALUE, 
    CR,
    LF,
    END_CR, 
};



class HttpParser {
public:
    HttpParser();
    ParseResult parse(const char* buffer, size_t len); 
    ParseResult parse_request_line();
    ParseResult parse_header();
    ParseResult parse_body();  
    void inspect(); 
    void reset(); 

private:

    // 记录相关数据
    // HTTP版本
    // HTTP方法
    // URI: 路径, 资源; 
    // headers, 头部(k-v)
    // 是否keepAlive. 
    // 请求体(消息体).

    struct Text {
        const char* pos;
        size_t len; 
    };

    struct Header {
        const char* key; 
        size_t k_len;
        const char* value;
        size_t v_len;
    };

    bool check_method(Text&); 
    bool check_uri(Text&);
    bool check_version(Text&);
    bool check_header(Header&); 

    ParseResult parse_result; 
    ParsePhase parse_phase; 
    ParseRequestLineState parse_rl_state;   
    ParseHeaderState parse_header_state; 

    Text method;  
    Text uri;
    Text version; 
    std::vector<Header> headers; 
    Text content; 
    size_t content_length;    // 消息体长度. (取自Content-Length字段)
    bool keep_alive;          
    const char* buf;          // 缓冲区起始位置
    size_t buf_len;           // 当前缓冲区中有效数据长度.
    const char* end;      // 缓冲区结束位置. 缓冲区有效部分: [buf, buf_end) 
    const char* pos;          // 当前位置. 
    bool done;                // 是否完成HTTP报文解析
};