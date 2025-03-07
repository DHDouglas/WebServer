#include <gtest/gtest.h> 

#include "http_parser.h"
#include "http_message.h"

using namespace std; 

/* 测试思路
 * 
 * 1) 实现一个测试夹具, 持有HttpParser, 提供测试函数:
 *    对指定地址的数据调用parser.parse()进行解析, 完成后根据解析结果构造一个Request对象.
 *    该对象提供一个encode()函数, 以string类型返回请求报文(包含\r\n等).
 * 
 * 2) 每个测试中, 手动构造一个Request对象作为答案, 调用add_method()等方法添加报文信息. 
 *    比较解析返回的Request对象与答案对象, 二者encode()返回的字符串内容是否一致.
 */ 


/* 测试项:
 * 
 * 1) 对"完整报文"的解析功能是否正常. 
 *   - 正确的HTTP报文: 无请求体、有请求体, 以及消息切割是否正确. 
 *   - 错误的HTTP报文: 
 *     - method错误; 
 *     - version错误; 
 * 
 * 2) 针对HTTP分包的多次重入解析. 
 *   - 头部不完整; 
 *   - 头部错误 
 *
 * 3) 测试对Response对象的构造. 
 */


class RequestFixture : public ::testing::Test {
protected:
    bool parse(const char* data, size_t& len, HttpRequest& request) {
        parser.parse(data, len); 
        return getParsedRequest(parser, request); 
    }

    bool getParsedRequest(HttpParser& parser, HttpRequest& request) {
        if (parser.parsing_completion()) {
            request.set_method(parser.get_method()); 
            request.set_uri(parser.get_uri()); 
            request.set_version(parser.get_version()); 
            request.set_headers(parser.get_headers()); 
            request.set_body(parser.get_body(), parser.get_body_size()); 
            return true; 
        }
        return false;
    }

    HttpParser parser; 
};



TEST_F(RequestFixture, TestParseRequestLine) {
    const char* text = "GET /uri?arg1=test;arg2=test HTTP/1.1\r\n\r\n"; 
    HttpRequest parsed;
    size_t consumed = strlen(text);  
    parse(text, consumed, parsed); 
    EXPECT_EQ(consumed, strlen(text));
    EXPECT_EQ(parsed.encode(), text); 
}


TEST_F(RequestFixture, TestParseHeaders) {
    const char* text = 
        "GET /uri?arg1=test;arg2=test HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Custom-Header1: value\r\n"
        "\r\n"; 
    HttpRequest parsed, expected;   
    size_t consumed = strlen(text);  
    parse(text, consumed, parsed); 
    EXPECT_EQ(consumed, strlen(text));
    EXPECT_EQ(parsed.encode(), text); 
}


TEST_F(RequestFixture, TestParseBody) {
    const char* text = 
        "GET /uri?arg1=test;arg2=test HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Custom-Header1: value\r\n"
        "Content-Length: 12\r\n"
        "\r\n"
        "Hello World!"; 
    HttpRequest parsed, expected;   
    size_t consumed = strlen(text);  
    parse(text, consumed, parsed); 
    EXPECT_EQ(consumed, strlen(text));
    EXPECT_EQ(parsed.encode(), text); 
}


TEST_F(RequestFixture, TestHttpRequest) {
    const char* text = 
        "GET /index/html HTTP/1.1\r\n"
        "Host: img.mukewang.com\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64)"
        "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\n"
        "Accept: image/webp,image/*,*/*;q=0.8\r\n"
        "Referer: http://www.imooc.com/\r\n"
        "Accept-Encoding: gzip, deflate, sdch\r\n"
        "Accept-Language: zh-CN,zh;q=0.8\r\n"
        "Content-length: 12\r\n"
        "\r\n"
        "Hello, World"; 

    HttpRequest parsed;
    size_t consumed = strlen(text);  
    parse(text, consumed, parsed); 
    EXPECT_EQ(consumed, strlen(text));
    EXPECT_EQ(parsed.encode(), text); 
}



TEST_F(RequestFixture, TestPartion) {
    const char* text = 
        "GET /index/html HTTP/1.1\r\n"
        "Host: img.mukewang.com\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64)"
        "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\n"
        "Accept: image/webp,image/*,*/*;q=0.8\r\n"
        "Referer: http://www.imooc.com/\r\n"
        "Accept-Encoding: gzip, deflate, sdch\r\n"
        "Accept-Language: zh-CN,zh;q=0.8\r\n"
        "Content-length: 12\r\n"
        "\r\n"
        "Hello, World"; 

    size_t len = strlen(text); 
    size_t delta = 1.0/6 * len; 
    size_t consumed = delta; 
    size_t buf_size = 0;  

    using Result = HttpParser::ParseResult;
    Result ret; 
    for (size_t pos = 0; pos < len; ) {
        buf_size += delta; 
        consumed = buf_size;
        ret = parser.parse(text + pos, consumed); 
        if (pos + consumed >= len) {
            EXPECT_EQ(ret, Result::SUCCESS); 
            EXPECT_EQ(pos + consumed, len);
            EXPECT_EQ(parser.encode(), text);  
        } else {
            EXPECT_EQ(ret, Result::AGAIN); 
        }        
        pos += consumed; 
        buf_size -= consumed; 
    }
}


TEST_F(RequestFixture, TestReuse) {
    using Result = HttpParser::ParseResult;
    Result ret; 

    const char* text1 = 
        "GET /uri?arg1=test;arg2=test HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Custom-Header1: value\r\n"
        "Content-Length: 12\r\n"
        "\r\n"
        "Hello World!";
    size_t len = strlen(text1);  
    ret = parser.parse(text1, len);
    EXPECT_EQ(len, strlen(text1));
    EXPECT_EQ(ret, Result::SUCCESS);  
    EXPECT_EQ(parser.encode(), text1);


    const char* text2 = 
        "GET /index/html HTTP/1.1\r\n"
        "Host: img.mukewang.com\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64)"
        "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\n"
        "Accept: image/webp,image/*,*/*;q=0.8\r\n"
        "Referer: http://www.imooc.com/\r\n"
        "Accept-Encoding: gzip, deflate, sdch\r\n"
        "Accept-Language: zh-CN,zh;q=0.8\r\n"
        "Content-length: 10\r\n"
        "\r\n"
        "ABCDEFGHIJ"; 
    len = strlen(text2);
    ret = parser.parse(text2, len);
    EXPECT_EQ(len, strlen(text2));
    EXPECT_EQ(ret, Result::SUCCESS);  
    EXPECT_EQ(parser.encode(), text2);

    const char* text3 = "GET /uri?arg1=test;arg2=test HTTP/1.1\r\n\r\n"; 
    len = strlen(text3);
    ret = parser.parse(text3, len);
    EXPECT_EQ(len, strlen(text3));
    EXPECT_EQ(ret, Result::SUCCESS);  
    EXPECT_EQ(parser.encode(), text3);

    const char* text4 = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nContent-Length: 17\r\n\r\nBody content here"; 
    len = strlen(text4);
    ret = parser.parse(text4, len);
    EXPECT_EQ(len, strlen(text4));
    EXPECT_EQ(ret, Result::SUCCESS);  
    EXPECT_EQ(parser.encode(), text4);
}

