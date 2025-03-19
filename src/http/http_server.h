#pragma once

#include <cstdint>
#include <memory>

#include "tcp_server.h"
#include "eventloop.h"
#include "tcp_connection.h"
#include "async_logger.h"
#include "config.h"
#include "timing_wheel.h"

class HttpConnection;

class HttpServer {
public:
    explicit HttpServer(const Config& config);     // 从buffer定期刷入日志文件的间隔秒数. 

    void start(); 

private:
    using TcpConnectionPtr = TcpServer::TcpConnectionPtr;
    using HttpConnectionPtr = std::shared_ptr<HttpConnection>;

private:
    // 注册于TcpConnection中的回调
    void onConnection(const TcpConnectionPtr& conn); 
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receive_time); 

    // 定时器超时回调.
    void onTimer(Any& data);

    // 日志输出回调, 指定输出位置. (stdout 或 )
    void logOutputToFile(const char* msg, int len); 

    EventLoop loop_; 
    Config config_;                // 配置项               
    std::unique_ptr<AsyncLogger> async_logger_;  
    TcpServer tcp_server_;
    std::atomic<uint32_t> num_connected_;
};