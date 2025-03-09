#pragma once

#include <memory>
#include "inet_address.h"
#include "tcp_server.h"
#include "tcp_connection.h"
#include "http_connection.h"
#include "logger.h"
#include "async_logger.h"


class HttpServer {
public:
    explicit HttpServer(const std::string ip, 
                        const int port, 
                        int num_thread,
                        std::string root_path,
                        double timeout_seconds,
                        std::string log_file_name = "HttpServerLog",   
                        Logger::LogLevel log_level = Logger::INFO,   
                        off_t log_rollsize = 500 * 1000 * 1000,  // 单份日志上限(满后切换新文件),  0.5GB
                        int log_flush_interval_seconds = 2);     // 从buffer定期刷入日志文件的间隔秒数. 

    void start(); 

private:
    using TcpConnectionPtr = TcpServer::TcpConnectionPtr;
    using HttpConnectionPtr = std::shared_ptr<HttpConnection>;

private:
    void processResourcesRootPath();
    // 注册于TcpConnection中的回调
    void onConnection(const TcpConnectionPtr& conn); 
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receive_time); 

    // 日志输出回调, 指定输出位置. (stdout 或 )
    void logOutputToFile(const char* msg, int len); 

    EventLoop loop_; 
    InetAddress addr_;             // ip, port
    std::string root_path_;        // 资源根目录. 

    Logger::LogLevel log_level_; 
    std::unique_ptr<AsyncLogger> async_logger_;  

    Duration timeout_duration_;    // http连接的keep-alive时间
    TcpServer tcp_server_;
    std::map<TcpConnectionPtr, HttpConnectionPtr> maps_; 
};