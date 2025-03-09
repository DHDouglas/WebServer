#pragma once

#include "buffer.h"
#include "channel.h"
#include "inet_address.h"
#include "tcp_server.h"
#include "tcp_connection.h"
#include "http_connection.h"
#include "logger.h"
#include "http_message.h"


class HttpServer {
public:
    explicit HttpServer(const std::string ip, 
                        const int port, 
                        int num_thread,
                        std::string root_path,
                        double timeout_seconds = 20);

    void start(); 

private:
    using TcpConnectionPtr = TcpServer::TcpConnectionPtr;
    using HttpConnectionPtr = std::shared_ptr<HttpConnection>;

private:
    void processResourcesRootPath();
    // 注册于TcpConnection中的回调
    void onConnection(const TcpConnectionPtr& conn); 
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receive_time); 

    // 应用层业务处理回调 => 应交由线程池处理.
    void onRequest(const TcpConnectionPtr& tcp_conn, const HttpConnectionPtr& http_conn);
    // void parseUrl(const std::string& url); 
    HttpStatusCode processPath(std::string& path); 
    void sendResponse(const TcpConnectionPtr& tcp_conn,  const HttpResponse& response); 

    void shutdown(const HttpConnection& http_conn); 
    
    void checkFile(const std::string file_path); 

    // 映射文件到内存. RAII封装
    struct MmapData {
        explicit MmapData(): fd_(-1), addr_(nullptr), size_(0) { }
        bool mmap(const std::string& file_path); 
        ~MmapData();
        int fd_;
        void* addr_;
        size_t size_;
    };

    EventLoop loop_; 
    InetAddress addr_;
    std::string root_path_;   // 资源根目录. 
    // Logger::LogLevel log_level_; 
    Duration timeout_duration_;  // http连接的keep-alive时间
    TcpServer tcp_server_;
    std::map<TcpConnectionPtr, HttpConnectionPtr> maps_; 
};