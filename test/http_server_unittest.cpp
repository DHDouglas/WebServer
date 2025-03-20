#include <gtest/gtest.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <memory>
#include <thread>

#include "config.h"
#include "http_server.h"
#include "http_connection.h"
#include "http_message.h"
#include "inet_address.h"

using namespace std;

/* 针对HttpServer处理静态资源请求的单元测试
 *
 * 测试思路:
 * 1. 定义一个Gtest测试环境类, 在执行所有测试用例前先启动HttpServer.
 * 2. 每个测试用例下, 一个独立Client向HttpServer建立连接, 发送请求, 而后阻塞等待获取响应报文.
 * 3. EXPECT_EQ对比收到的响应报文与预期报文是否一致.
 * 
 * 测试内容:
 * 1. HTTP1.0默认短连接, HTTP1.0启用长连接
 * 2. HTTP1.1默认长连接, HTTP1.1启用短连接
 * 3. HTTP1.1 GET, 特殊URL, 非静态资源文件, 直接响应报文, 不走磁盘I/O
 * 4. HTTP1.1 GET/HEAD, 请求静态资源文件, 200
 * 5. HTTP1.1 GET/HEAD, 文件不存在, 404
 * 6. HTTP1.1 请求错误, 400 
 * 6. HTTP1.1 未实现方法, 501 (项目里只实现了GET与HEAD, 其余方法均响应501)
 */

class Client {
public:
    Client(string ip, uint16_t port) :addr_(ip, port), sockfd(-1) {
        buffer = make_unique<char[]>(BUF_SIZE); 
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            ADD_FAILURE() << "Client sockfd failed"; 
        }
        if (connect(sockfd, addr_.getSockAddr(), addr_.getAddrLen()) < 0) {
            ADD_FAILURE() << "Client connect failed"; 
        }
    }

    ~Client() {
        if (sockfd != -1) ::close(sockfd); 
    }

    void send(const char* msg, int len) {
        if (sockfd != -1) {
            ::send(sockfd, msg, len, 0);
        } else {
            ADD_FAILURE() << "send: invalid sockfd";
        }
    }

    void read() {
        if (sockfd == -1)  ADD_FAILURE() << "read: invalid sockfd";
        int n_read = ::read(sockfd, buffer.get(), BUF_SIZE); 
        if (n_read < 0) {
            ADD_FAILURE() << "Client read failed"; 
        }
        buffer[n_read] = '\0'; 
        end = n_read;
    }

    string getResponseAsString() {
        return buffer.get();
    }

    vector<char> getResponseAsBin() {
        return vector<char>(buffer.get(), buffer.get() + end);
    }

private:
    static constexpr int BUF_SIZE = 1000 * 1000;
    InetAddress addr_; 
    unique_ptr<char[]> buffer; 
    int end; 
    int sockfd; 
};


// 定义测试环境类
class HttpServerEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        std::thread t(&HttpServerEnvironment::runServer); 
        std::this_thread::sleep_for(chrono::milliseconds(500));  // 等待服务器启动
        t.detach();
    }

    void TearDown() override {}
    
    static void runServer() {
        Config config;
        config.port = 8080;  
        config.log_enable = false;
        config.root_path_ = root_path_; 
        HttpServer server(config);
        server.start();
    }

    static vector<char> loadFile(string url) {
        MmapData mmapdata;
        if (mmapdata.mmap(root_path_ + url)) {
            return vector<char>(
                static_cast<char*>(mmapdata.addr_),
                static_cast<char*>(mmapdata.addr_) + mmapdata.size_);
        }
        return {};
    }

private:
    static string root_path_; 
};

string HttpServerEnvironment::root_path_ = "/home/yht/Projects/MyTinyWebServer/resources"; 


string makeRequest(string url, HttpMethod method, string version, bool keep_alive) {
    char buffer[128] {};
    string fmt = "%s %s %s\r\n";
    if (keep_alive) {
        if (strncasecmp("HTTP/1.0", version.c_str(), 8) == 0) {
            fmt += "Connection: keep-alive\r\n"; 
        }
    } else {
        if (strncasecmp("HTTP/1.1", version.c_str(), 8) == 0) {
            fmt += "Connection: close\r\n"; 
        }
    }
    fmt += "\r\n";
    snprintf(buffer, sizeof buffer, fmt.c_str(), 
        getHttpMethodAsString(method).c_str(), url.c_str(), version.c_str());
    return buffer;
}

vector<char> makeReponse(string url, HttpMethod method, string version, bool keep_alive) {
    HttpStatusCode code; 
    vector<char> content;
    char buffer[512] {};
    string fmt = "%s %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n";
    string fmt_no_content = "%s %s\r\nContent-Length: 0\r\n";

    if (keep_alive) {
        if (strncasecmp("HTTP/1.0", version.c_str(), 8) == 0) {
            fmt += "Connection: keep-alive\r\n"; 
            fmt_no_content += "Connection: keep-alive\r\n"; 
        }
    } else {
        if (strncasecmp("HTTP/1.1", version.c_str(), 8) == 0) {
            fmt += "Connection: close\r\n"; 
            fmt_no_content += "Connection: close\r\n"; 
        }
    }
    fmt += "\r\n";
    fmt_no_content += "\r\n"; 

    if (url.empty()) {  // 400: bad request
        code = HttpStatusCode::BadRequest; 
        url = "/400.html"; 
        content = HttpServerEnvironment::loadFile(url); 
        if (!content.empty()) {  
            snprintf(buffer, sizeof buffer, fmt.c_str(), 
                version.c_str(), 
                getHttpStatusCodePhaseString(code).c_str(), 
                getContentType(url).c_str(),
                content.size());
        } else {  // 错误页面文件也不存在. 
            snprintf(buffer, sizeof buffer, fmt_no_content.c_str(), 
                     version.c_str(),
                     getHttpStatusCodePhaseString(code).c_str());
        }
    } else {
        if (method == HttpMethod::GET || method == HttpMethod::HEAD) {
            content = HttpServerEnvironment::loadFile(url); 
            if (!content.empty()) {  // 资源文件存在
                code = HttpStatusCode::OK; 
                snprintf(buffer, sizeof buffer, fmt.c_str(),
                    version.c_str(), 
                    getHttpStatusCodePhaseString(code).c_str(), 
                    getContentType(url).c_str(),
                    content.size());
            } else { 
                code = HttpStatusCode::NotFound; 
                url = "/404.html"; 
                content = HttpServerEnvironment::loadFile(url); 
                if (!content.empty()) {  
                    snprintf(buffer, sizeof buffer, fmt.c_str(), 
                        version.c_str(), 
                        getHttpStatusCodePhaseString(code).c_str(), 
                        getContentType(url).c_str(),
                        content.size());
                } else {  // 错误页面文件也不存在. 
                    snprintf(buffer, sizeof buffer, fmt_no_content.c_str(), 
                             version.c_str(),
                             getHttpStatusCodePhaseString(code).c_str());
                }
            }
        } else {  // 501 Not Implemented
            code = HttpStatusCode::NotImplemented;
            url = "/501.html"; 
            content = HttpServerEnvironment::loadFile(url); 
            if (!content.empty()) {  
                snprintf(buffer, sizeof buffer, fmt.c_str(), 
                version.c_str(), 
                getHttpStatusCodePhaseString(code).c_str(), 
                getContentType(url).c_str(),
                content.size());
            } else {  // 错误页面文件也不存在. 
                snprintf(buffer, sizeof buffer, fmt_no_content.c_str(), 
                         version.c_str(), 
                         getHttpStatusCodePhaseString(code).c_str());
            }
        }
    }

    vector<char> response(buffer, buffer + strlen(buffer));
    if (!content.empty() && method != HttpMethod::HEAD) {
        response.insert(response.end(), content.begin(), content.end()); 
    } 
    return response;
}



// 测试HTTP1.0短连接
TEST(HttpServer, Test_10_GET_benchmark) {
    Client client("localhost", 8080);
    const char* request = "GET /benchmark HTTP/1.0\r\n\r\n"; 
    const char* expect_response = 
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello World!";
    client.send(request, strlen(request)); 
    client.read(); 
    string resp = client.getResponseAsString();
    EXPECT_EQ(resp, expect_response) << resp; 

    // 此时连接应已关闭, read==0.
    client.send(request, strlen(request)); 
    client.read(); 
    resp = client.getResponseAsString();
    EXPECT_EQ(resp, "") << resp; 
}

// 测试HTTP1.0长连接
TEST(HttpServer, Test_10_GET_benchmark_keepalive) {
    Client client("localhost", 8080);
    const char* request = 
        "GET /benchmark HTTP/1.0\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    const char* expect_response = 
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "\r\n"
        "Hello World!";
    client.send(request, strlen(request)); 
    client.read(); 
    string resp = client.getResponseAsString();
    EXPECT_EQ(resp, expect_response) << resp; 
}

// 测试HTTP1.1短连接
TEST(HttpServer, Test_11_GET_benchmark_close) {
    Client client("localhost", 8080);
    const char* request = 
        "GET /benchmark HTTP/1.1\r\n"
        "Connection: close\r\n"
        "\r\n"; 
    const char* expect_response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello World!";
    client.send(request, strlen(request)); 
    client.read(); 
    string resp = client.getResponseAsString();
    EXPECT_EQ(resp, expect_response) << resp; 

    // 此时连接应已关闭, read==0.
    client.send(request, strlen(request)); 
    client.read(); 
    resp = client.getResponseAsString();
    EXPECT_EQ(resp, "") << resp; 
}


// 测试HTTP1.1长连接
TEST(HttpServer, Test_11_GET_benchmark) {
    Client client("localhost", 8080);
    const char* request = "GET /benchmark HTTP/1.1\r\n\r\n"; 
    const char* expect_response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "\r\n"
        "Hello World!"; 
    client.send(request, strlen(request)); 
    client.read(); 
    string resp = client.getResponseAsString();
    EXPECT_EQ(resp, expect_response) << resp; 

    client.send(request, strlen(request)); 
    client.read(); 
    resp = client.getResponseAsString();
    EXPECT_EQ(resp, expect_response) << resp; 
}


// 测试HTTP1.1, HEAD请求
TEST(HttpServer, Test_11_HEAD_benchmark) {
    Client client("localhost", 8080);
    const char* request = "HEAD /benchmark HTTP/1.1\r\n\r\n"; 
    const char* expect_response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "\r\n";
    client.send(request, strlen(request)); 
    client.read(); 
    string resp = client.getResponseAsString();
    EXPECT_EQ(resp, expect_response) << resp; 

    client.send(request, strlen(request)); 
    client.read(); 
    resp = client.getResponseAsString();
    EXPECT_EQ(resp, expect_response) << resp; 
}



// 测试HTTP1.1, GET请求静态资源
TEST(HttpServer, Test_11_GET_file) {
    Client client("localhost", 8080);
    string url;
    string version = "HTTP/1.1";
    HttpMethod method = HttpMethod::GET;
    string request;
    vector<char> expect_response;
    vector<char> resp;

    url = "/";
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse("/index.html", method, version, true); 

    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/test.txt"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/index.html"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/picture.html"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/video.html"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/images/favicon.ico"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/images/instagram-image2.jpg"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;
}


// 测试HTTP1.1, HEAD请求静态资源
TEST(HttpServer, Test_11_HEAD_file) {
    Client client("localhost", 8080);
    string url;
    string version = "HTTP/1.1";
    HttpMethod method = HttpMethod::HEAD;
    string request;
    vector<char> expect_response;
    vector<char> resp;

    url = "/";
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse("/index.html", method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/test.txt"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/index.html"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/picture.html"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/video.html"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/images/favicon.ico"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/images/instagram-image2.jpg"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/video/xxx.mp4"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;
}


// 测试HTTP1.1, GET请求静态资源, 404 
TEST(HttpServer, Test_11_GET_404) {
    Client client("localhost", 8080);
    string url;
    string version = "HTTP/1.1";
    HttpMethod method = HttpMethod::GET;
    string request;
    vector<char> expect_response;
    vector<char> resp;

    url = "/ABD_NOT_EXIST.txt"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/GGGG.html"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/FF"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;

    url = "/iSSSS"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << url;
}


// 测试HTTP1.1, HEAD请求静态资源, 404 
TEST(HttpServer, Test_11_HEAD_404) {
    Client client("localhost", 8080);
    string url;
    string version = "HTTP/1.1";
    HttpMethod method = HttpMethod::HEAD;
    string request;
    vector<char> expect_response;
    vector<char> resp;

    url = "/ABD_NOT_EXIST.txt"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response);

    url = "/GGGG.html"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response);

    url = "/FF"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response);

    url = "/iSSSS"; 
    request = makeRequest(url, method, version, true);
    expect_response = makeReponse(url, method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response);
}

// 测试HTTP1.1, 请求错误 400 
TEST(HttpServer, Test_11_GET_400) {
    Client client("localhost", 8080);
    string url;
    string version = "HTTP/1.1";
    HttpMethod method = HttpMethod::GET;
    string request;
    vector<char> expect_response;
    vector<char> resp;
    
    request = "bad http request\r\n\r\n"; 
    expect_response = makeReponse("", method, version, false); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response);
    // cout << resp.size() << " " << expect_response.size() << endl;
    // cout << string(resp.begin(), resp.end()) << endl;
    // cout << string(expect_response.begin(), expect_response.end()) << endl;
    
    // 错误请求后, 会被关闭连接, 因此再发送正确请求也无效.
    request = "GET /test.txt HTTP/1.1\r\n\r\n"; 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, vector<char>{});
}


// 测试HTTP1.1, 未定义请求 501
// 只实现了GET、HEAD.
TEST(HttpServer, Test_11_GET_501) {
    Client client("localhost", 8080);
    string url;
    string version = "HTTP/1.1";
    HttpMethod method = HttpMethod::UNKNOWN;
    string request;
    vector<char> expect_response;
    vector<char> resp;
    
    request = "POST /test.txt HTTP/1.1\r\n\r\n"; 
    expect_response = makeReponse("/test.txt", method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << request;

    request = "POST /benchmark HTTP/1.1\r\n\r\n"; 
    expect_response = makeReponse("/benchmark", method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << request;
    
    request = "PUT /test.txt HTTP/1.1\r\n\r\n"; 
    expect_response = makeReponse("/test.txt", method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << request;

    request = "PUT /benchmark HTTP/1.1\r\n\r\n"; 
    expect_response = makeReponse("/benchmark", method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << request;

    request = "DELETE /test.txt HTTP/1.1\r\n\r\n"; 
    expect_response = makeReponse("/test.txt", method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << request;

    request = "DELETE /benchmark HTTP/1.1\r\n\r\n"; 
    expect_response = makeReponse("/benchmark", method, version, true); 
    client.send(request.c_str(), request.length()); 
    client.read(); 
    resp = client.getResponseAsBin();
    EXPECT_EQ(resp, expect_response) << request;
}


int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new HttpServerEnvironment);  // 注册测试环境
    return RUN_ALL_TESTS();
}