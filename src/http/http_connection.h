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

    // 处理&解析HTTP请求报文
    void handleMessage(Buffer* buf);

private:
    // 业务逻辑 
    void handleRequest();
    
    // 处理url路径, 若有效且目标文件存在&可访问, 则path设置为文件的绝对路径.
    HttpStatusCode processPath(string& path);

    // 重置定时器
    void restartTimer(); 
    // 仅关闭写端. 
    void shutdown() const;
    // 断开连接
    void forceClose() const;

    void sendErrorResponse(const HttpStatusCode& code);
    void sendResponse(const HttpResponse& response);     // base.

private:
    // 映射文件到内存. RAII封装
    struct MmapData {
        explicit MmapData(): fd_(-1), addr_(nullptr), size_(0) { }
        bool mmap(const std::string& file_path); 
        ~MmapData();
        int fd_;
        void* addr_;
        size_t size_;
    };

    HttpParser parser_;
    Duration timeout_duration_;
    std::string root_path_; 
    std::weak_ptr<TcpConnection> tcp_conn_wkptr; 
    std::weak_ptr<Timer> timer_wkptr_; 
};