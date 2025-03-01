


// TEST_F(RequestFixture, TestParseRequestLine) {
//     const char* text = "GET /uri?arg1=test;arg2=test HTTP/1.1\r\n\r\n"; 
//     HttpRequest parsed, expected;   
//     parse(text, strlen(text), parsed); 

//     expected.set_uri("/uri?arg1=test;arg2=test");
//     expected.set_method("GET"); 
//     expected.set_version("HTTP/1.1"); 
//     // EXPECT_EQ(parsed.encode(), expected.encode()); 

//     EXPECT_EQ(parsed.encode(), text); 
// }


// TEST_F(RequestFixture, TestParseHeaders)  {
//     const char* text = 
//         "GET /uri?arg1=test;arg2=test HTTP/1.1\r\n"
//         "Host: 127.0.0.1\r\n"
//         "Custom-Header1: value\r\n"
//         "\r\n"; 
//     HttpRequest parsed, expected;   
//     parse(text, strlen(text), parsed); 

//     expected.set_uri("/uri?arg1=test;arg2=test");
//     expected.set_method("GET"); 
//     expected.set_version("HTTP/1.1"); 
//     expected.add_header("Host", "127.0.0.1");
//     expected.add_header("Custom-Header1", "value"); 
//     EXPECT_EQ(parsed.encode(), expected.encode()); 
// }


// TEST_F(RequestFixture, TestParseBody)  {
//     const char* text = 
//         "GET /uri?arg1=test;arg2=test HTTP/1.1\r\n"
//         "Host: 127.0.0.1\r\n"
//         "Custom-Header1: value\r\n"
//         "Content-Length: 12\r\n"
//         "\r\n"
//         "Hello World!"; 
//     HttpRequest parsed, expected;   
//     parse(text, strlen(text), parsed); 

//     expected.set_uri("/uri?arg1=test;arg2=test");
//     expected.set_method("GET"); 
//     expected.set_version("HTTP/1.1"); 
//     expected.add_header("Host", "127.0.0.1");
//     expected.add_header("Custom-Header1", "value"); 
//     expected.set_body("Hello World!", 12); 
//     EXPECT_EQ(parsed.encode(), expected.encode()); 
// }


// TEST_F(RequestFixture, TestHttpRequest)  {
//     const char* text = 
//         "GET /index/html HTTP/1.1\r\n"
//         "Host: img.mukewang.com\r\n"
//         "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64)"
//         "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\n"
//         "Accept: image/webp,image/*,*/*;q=0.8\r\n"
//         "Referer: http://www.imooc.com/\r\n"
//         "Accept-Encoding: gzip, deflate, sdch\r\n"
//         "Accept-Language: zh-CN,zh;q=0.8\r\n"
//         "Content-length: 12\r\n"
//         "\r\n"
//         "Hello, World"; 

//     HttpRequest parsed, expected;   
//     parse(text, strlen(text), parsed); 

//     expected.set_method("GET"); 
//     expected.set_uri("/index/html");
//     expected.set_version("HTTP/1.1"); 
//     expected.add_header("Host", "img.mukewang.com");
//     expected.add_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64)AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36");
//     expected.add_header("Accept", "image/webp,image/*,*/*;q=0.8"); 
//     expected.add_header("Accept", "image/webp,image/*,*/*;q=0.8"); 
//     expected.set_body("Hello World!", 12); 
//     EXPECT_EQ(parsed.encode(), expected.encode()); 
// }



