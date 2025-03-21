#include "http_server.h"

#include <functional>

#include "http_connection.h"
#include "inet_address.h"
#include "logger.h"
#include "tcp_server.h"
#include "timestamp.h"
#include "any.h"
#include "timing_wheel.h"

using namespace std;


HttpServer::HttpServer(const Config& config)
    : config_(config),
    tcp_server_(&loop_, config.ip.empty()?InetAddress(config.port):InetAddress(config.port), "HttpServer"),
    num_connected_(0)
{   
    // Logger配置
    Logger::setLogLevel(config.log_level_);
    if (config.log_enable) {
        Logger::enableLogger(true); 
        // 输出日志到文件. 默认输出到stdout.
        if (!config.log_file_name_.empty() &&
            config.log_rollsize_ > 0 && 
            config.log_flush_interval_seconds_ > 0) 
        {  
            async_logger_ = make_unique<AsyncLogger>(
                config.log_file_name_, 
                config.log_dir_, 
                config.log_rollsize_, 
                config.log_flush_interval_seconds_); 
            Logger::setOutput(bind(&HttpServer::logOutputToFile, this, placeholders::_1, placeholders::_2)); 
        }
    } else {
        Logger::enableLogger(false);   // 关闭日志输出.
    }
    // TcpServer配置
    tcp_server_.setThreadNum(config.num_thread); 
    tcp_server_.setConnectionCallback(
        bind(&HttpServer::onConnection, this, placeholders::_1)); 
    tcp_server_.setMessageCallback(
        bind(&HttpServer::onMessage, this, placeholders::_1,
             placeholders::_2, placeholders::_3));
    tcp_server_.setWriteCompleteCallback(
        bind(&HttpServer::onWriteComplete, this, placeholders::_1));
    // HttpConn配置
    HttpConnection::setRootPath(config.root_path_); 
    HttpConnection::setTimeout(config.timeout_seconds_); 
}


void HttpServer::start() {
    LOG_INFO << "HttpServer starts listening on " << tcp_server_.getIpPort();
    printf("HttpServer starts listening on %s\n", tcp_server_.getIpPort().c_str()); 
    config_.printArgs();
    if (async_logger_) {
        async_logger_->start(); 
    }
    tcp_server_.start();
    // 各IO线程EventLoop绑定时间轮.
    if (config_.timeout_seconds_ > 0) {
        auto loops = tcp_server_.getEventLoopPool()->getAllLoops(); 
        for (auto& loop : loops) {
            loop->setContext(make_unique<TimingWheel>(
                loop, 
                config_.timeout_seconds_, 
                HttpConnection::onTimer
            ));
        }
    }
    loop_.loop();
}


void HttpServer::onConnection(const TcpServer::TcpConnectionPtr& tcp_conn) {
    LOG_INFO << "HttpConnection:" << tcp_conn->getPeerAddress().getIpPortString() 
             << " is " << (tcp_conn->connected() ? "UP" : "DOWN"); 
    if (tcp_conn->connected()) {
        ++num_connected_;
        if (num_connected_ > config_.max_connections_) {
            tcp_conn->shutdown();
            tcp_conn->forceClose(); 
        }
        // TcpConnection的context_以`shared_ptr`的形式绑定HttpConnection, 
        // 旨在让HttpConnection的回调函数能通过weak_ptr<HttpConnection>的形式绑定而不是通过HttpConnection::this, 
        // 防止TcpConnection析构后HttpConnection也失效, 回调实际执行时触发段错误.
        tcp_conn->setContext(make_shared<HttpConnection>(tcp_conn));
    } else {
        --num_connected_;
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

void HttpServer::onWriteComplete(const TcpServer::TcpConnectionPtr& tcp_conn) {
    auto http_conn = any_cast<shared_ptr<HttpConnection>>(tcp_conn->getMutableContext());
    assert(http_conn);
    (*http_conn)->onWriteComplete();
}

void HttpServer::logOutputToFile(const char* msg, int len) {
    async_logger_->append(msg, len); 
}
