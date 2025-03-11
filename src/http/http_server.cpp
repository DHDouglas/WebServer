#include "http_server.h"

#include <functional>

#include "http_connection.h"
#include "inet_address.h"
#include "logger.h"
#include "timestamp.h"
#include "any.h"

using namespace std;


HttpServer::HttpServer(const Config& config)
    : config_(config),
    tcp_server_(&loop_, config.ip.empty()?InetAddress(config.port):InetAddress(config.port), "HttpServer")
{   
    Logger::setLogLevel(config.log_level_);
    // 记录日志到文件. 默认输出到stdout.
    if (!config.log_file_name_.empty() &&
         config.log_rollsize_ > 0 && 
         config.log_flush_interval_seconds_ > 0) {  
        async_logger_ = make_unique<AsyncLogger>(
            config.log_file_name_, 
            config.log_dir_, 
            config.log_rollsize_, 
            config.log_flush_interval_seconds_); 
        Logger::setOutput(bind(&HttpServer::logOutputToFile, this, placeholders::_1, placeholders::_2)); 
    }

    tcp_server_.setThreadNum(config.num_thread); 
    tcp_server_.setConnectionCallback(
        bind(&HttpServer::onConnection, this, placeholders::_1)); 
    tcp_server_.setMessageCallback(
        bind(&HttpServer::onMessage, this, placeholders::_1,
             placeholders::_2, placeholders::_3));
}


void HttpServer::start() {
    LOG_INFO << "HttpServer starts listening on " << tcp_server_.getIpPort();
    printf("HttpServer starts listening on %s\n", tcp_server_.getIpPort().c_str()); 
    config_.printArgs();
    if (async_logger_) {
        async_logger_->start(); 
    }
    tcp_server_.start();
    loop_.loop();
}


void HttpServer::onConnection(const TcpServer::TcpConnectionPtr& tcp_conn) {
    if (tcp_conn->connected()) {
        LOG_TRACE << "HttpServer::onConnection: new HttpConnection"; 
        // TcpConnection的context_以`shared_ptr`的形式绑定HttpConnection, 
        // 旨在让HttpConnection的回调函数能通过weak_ptr<HttpConnection>的形式绑定而不是通过HttpConnection::this, 
        // 防止TcpConnection析构后HttpConnection也失效, 回调实际执行时触发段错误.
        tcp_conn->setContext(make_shared<HttpConnection>(
            tcp_conn, 
            config_.root_path_, 
            Timestamp::secondsToDuration(config_.timeout_seconds_)
        ));
    } else {
        LOG_TRACE << "HttpServer::onConnection: remove tcp_conn";
    }
}


void HttpServer::onMessage(const TcpServer::TcpConnectionPtr& tcp_conn,
                           Buffer* buf, 
                           Timestamp receive_time)
{
    LOG_TRACE << "HttpServer::onMessage receive_time: " << receive_time.toFormattedString().c_str(); 
    auto http_conn = any_cast<shared_ptr<HttpConnection>>(tcp_conn->getMutableContext());
    assert(http_conn);
    (*http_conn)->handleMessage(buf);
}


void HttpServer::logOutputToFile(const char* msg, int len) {
    async_logger_->append(msg, len); 
}
