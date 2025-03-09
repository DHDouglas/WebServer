#include "http_server.h"
#include "http_connection.h"
#include "inet_address.h"
#include "logger.h"
#include "tcp_server.h"

#include <fcntl.h>
#include <functional>
#include <linux/limits.h>    // for PATH_MAX
#include <sys/mman.h>
#include <sys/stat.h>
#include "timestamp.h"


using namespace std;


HttpServer::HttpServer(const string ip, 
                       const int port, 
                       int num_thread,
                       string root_path,
                       double timeout_seconds,
                       string log_file_name,
                       Logger::LogLevel log_level,
                       off_t log_rollsize, 
                       int log_flush_inerval_seconds)

    : addr_(ip.empty() ? InetAddress(port) : InetAddress(ip, port)),
    root_path_(root_path),
    timeout_duration_(Timestamp::secondsToDuration(timeout_seconds)),
    tcp_server_(&loop_, addr_, "HttpServer")
{   
    Logger::setLogLevel(log_level);
    // 记录日志到文件. 默认输出到stdout.
    if (!log_file_name.empty() && log_rollsize > 0 && log_flush_inerval_seconds > 0) {  
        async_logger_ = make_unique<AsyncLogger>(log_file_name, log_rollsize, log_flush_inerval_seconds); 
        Logger::setOutput(bind(&HttpServer::logOutputToFile, this, placeholders::_1, placeholders::_2)); 
    }

    processResourcesRootPath(); 
    tcp_server_.setThreadNum(num_thread); 
    tcp_server_.setConnectionCallback(
        bind(&HttpServer::onConnection, this, placeholders::_1)); 
    tcp_server_.setMessageCallback(
        bind(&HttpServer::onMessage, this, placeholders::_1,
             placeholders::_2, placeholders::_3));
}


void HttpServer::processResourcesRootPath() {
    // TODO: 区分是相对路径还是绝对路径. 
    // 如果是绝对路径, 直接赋给root_path_;
    // 如果是相对路径, 则调用getcwd获取当前工作目录, 再concat.
}


void HttpServer::start() {
    LOG_INFO << "HttpServer starts listening on " << tcp_server_.getIpPort();
    printf("HttpServer starts listening on %s\n", tcp_server_.getIpPort().c_str()); 
    if (async_logger_) {
        async_logger_->start(); 
    }
    tcp_server_.start();
    loop_.loop();
}

void HttpServer::onConnection(const TcpServer::TcpConnectionPtr& tcp_conn) {
    if (tcp_conn->connected()) {
        LOG_TRACE << "HttpServer::onConnection: new HttpConnection"; 
        auto http_conn = make_shared<HttpConnection>(tcp_conn, root_path_, timeout_duration_); 
        maps_[tcp_conn] = http_conn;
    } else {
        maps_.erase(tcp_conn); 
        LOG_TRACE << "HttpServer::onConnection: remove tcp_conn, remainder: " << maps_.size(); 
    }
}


void HttpServer::onMessage(const TcpServer::TcpConnectionPtr& tcp_conn,
                           Buffer* buf, 
                           Timestamp receive_time)
{
    LOG_TRACE << "HttpServer::onMessage receive_time: " << receive_time.toFormattedString().c_str(); 
    auto http_conn = maps_[tcp_conn]; 
    http_conn->handleMessage(buf);
}


void HttpServer::logOutputToFile(const char* msg, int len) {
    async_logger_->append(msg, len); 
}
