#include "http_parser.h"



int main() {
    // std::string request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\nBody content here";
    // HttpParser parser; 

    // 以string的形式提供? 或者以char buffer[]形式提供, 需要自行定位\r\n.  
    // httprequest.parse(request.c_str()); 

    // 初始化一个httpparser实例. 
    // 每次调用其进行解析前, 是不是应该更新缓冲区长度? 
    // => 确实, 每次需要传入新的缓冲区长度.

    // string str = "GET /index/html HTTP/1.1\r\n";
    // const char* buf = str.c_str(); 
    // ParseResult res;
    // res = parser.parse(buf, 5);
    // assert(res == ParseResult::AGAIN);
    // res = parser.parse(buf, strlen(buf)); 
    // assert(res == ParseResult::SUCCESS);
    // parser.inspect(); 

    // string str2 = 
    //   "Host: img.mukewang.com\r\n"
    //   "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64)"
    //   "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\n"
    //   "Accept: image/webp,image/*,*/*;q=0.8\r\n"
    //   "Referer: http://www.imooc.com/\r\n"
    //   "Accept-Encoding: gzip, deflate, sdch\r\n"
    //   "Accept-Language: zh-CN,zh;q=0.8\r\n\r\n";

    string str3 = 
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

    // const char* buf = str3.c_str();  
    

    // ParseResult res;
    // // res = parser.parse(buf, 5);
    // // assert(res == ParseResult::AGAIN);
    // res = parser.parse(buf, strlen(buf)); 
    // assert(res == ParseResult::SUCCESS);
    // parser.inspect(); 
    // httprequest.parse_header(str2.c_str(), str2.size()); 

    #define REQ                                                                                                                        \
    "GET /wp-content/uploads/2010/03/hello-kitty-darth-vader-pink.jpg HTTP/1.1\r\n"                                                \
    "Host: www.kittyhell.com\r\n"                                                                                                  \
    "User-Agent: Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; ja-JP-mac; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3 "             \
    "Pathtraq/0.9\r\n"                                                                                                             \
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"                                                  \
    "Accept-Language: ja,en-us;q=0.7,en;q=0.3\r\n"                                                                                 \
    "Accept-Encoding: gzip,deflate\r\n"                                                                                            \
    "Accept-Charset: Shift_JIS,utf-8;q=0.7,*;q=0.7\r\n"                                                                            \
    "Keep-Alive: 115\r\n"                                                                                                          \
    "Connection: keep-alive\r\n"                                                                                                   \
    "Cookie: wp_ozh_wsa_visits=2; wp_ozh_wsa_visit_lasttime=xxxxxxxxxx; "                                                          \
    "__utma=xxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.x; "                                                             \
    "__utmz=xxxxxxxxx.xxxxxxxxxx.x.x.utmccn=(referral)|utmcsr=reader.livedoor.com|utmcct=/reader/|utmcmd=referral\r\n"             \
    "\r\n"

    HttpParser parser;
    ParseResult res;

    res = parser.parse(REQ, sizeof(REQ) - 1);  
    assert(res == ParseResult::SUCCESS);
    parser.inspect();


    // auto start = std::chrono::steady_clock::now(); 
    
    // for (int i = 0; i < 10000000; i++) {
    // // for (int i = 0; i < 1; i++) {
    //     parser.reset();     
    //     res = parser.parse(REQ, sizeof(REQ) - 1); 
    //     assert(res == ParseResult::SUCCESS);
    // }

    // auto end = std::chrono::steady_clock::now(); 
    // std::chrono::duration<double> elapsed = end - start;
    // std::cout << elapsed.count() << std::endl;
    return 0;
}