#pragma once 

#include <memory>
#include <string>

#include "tcp_connection.h"
#include "buffer.h"
#include "timer.h"
#include "http_message.h"
#include "http_parser.h"


class HttpConnection {
public:
    using ParseResult = HttpParser::ParseResult; 

    HttpConnection(std::weak_ptr<TcpConnection> tcp_conn,
                   std::string root_path_, 
                   Duration timeout_duration);
    ~HttpConnection();
    // 处理&解析HTTP请求报文
    void handleMessage(Buffer* buf);

private:
    // 根据URL执行不同业务逻辑 
    void handleRequest();
    // 请求静态资源
    void responseMsg(const char* msg, size_t len, HttpStatusCode code);
    void responseFile(const string& path, HttpStatusCode code);  
    // 错误响应        
    void errorResponse(HttpStatusCode code);
    // 发送报文
    void sendResponse(const HttpResponse& response);     // base.
    // 定时器回调.
    void static handleTimer(const std::weak_ptr<TcpConnection>& wk_ptr);
    // 重置定时器
    void restartTimer(); 
    // 移除定时器
    void removeTimer();
    // 仅关闭写端. 
    void shutdown() const;
    // 断开连接
    void forceClose() const;

private:
    HttpParser parser_;
    Duration timeout_duration_;
    bool useTimeout_; 
    std::string root_path_; 
    std::weak_ptr<TcpConnection> tcp_conn_wkptr; 
    std::weak_ptr<Timer> timer_wkptr_; 
};


// 映射文件到内存. RAII封装
struct MmapData {
    explicit MmapData(): fd_(-1), addr_(nullptr), size_(0) { }
    ~MmapData();
    bool mmap(const std::string& file_path); 
    int fd_;
    void* addr_;
    size_t size_;
};
