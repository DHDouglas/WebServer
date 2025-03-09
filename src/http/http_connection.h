#pragma once 

#include <memory>
#include "http_parser.h"
#include "logger.h"
#include "tcp_connection.h"
#include "timer.h"
#include "eventloop.h"

class HttpConnection {
public:
    using ParseResult = HttpParser::ParseResult; 

    HttpConnection(std::weak_ptr<TcpConnection> tcp_conn,
                   Duration timeout_duration)
        : timeout_duration_(timeout_duration),
        tcp_conn_wkptr(tcp_conn)
    {
        auto conn_sptr = tcp_conn_wkptr.lock(); 
        if (conn_sptr) {
            LOG_TRACE << "HttpConnection: add Timer";
            timer_wkptr_ = conn_sptr->getOwnerLoop()->runAfter(
                timeout_duration_, bind(&HttpConnection::forceClose, this));
        }
    }

    std::string getRequestAsString() const {
        return http_parser.encode();
    }

    HttpParser& parser() {
        return http_parser;
    }


    void updateTimer() {
        auto conn_sptr = tcp_conn_wkptr.lock(); 
        auto timer_sptr = timer_wkptr_.lock();
        // conn或timer不存在, 均意味着TCP连接已断开. timer的定时任务即为forceClose掉Tcp连接.
        if (conn_sptr && timer_sptr) {
            LOG_TRACE << "HttpConnection::updateTimer";
            conn_sptr->getOwnerLoop()->removeTimer(timer_wkptr_); 
            timer_wkptr_ = conn_sptr->getOwnerLoop()->runAfter(
                timeout_duration_, bind(&HttpConnection::forceClose, this));
        }
    }

    void shutdown() const {
        auto conn_sptr = tcp_conn_wkptr.lock(); 
        if (conn_sptr) {
            conn_sptr->shutdown(); 
        }
    }

    void forceClose() const {
        auto conn_sptr = tcp_conn_wkptr.lock(); 
        auto timer_sptr = timer_wkptr_.lock();
        if (conn_sptr && timer_sptr) {
            // 清除定时器, 关闭tcp连接. 
            conn_sptr->getOwnerLoop()->removeTimer(timer_wkptr_); 
            conn_sptr->forceClose(); 
        }
    }

private:
    HttpParser http_parser;
    Duration timeout_duration_;
    std::weak_ptr<TcpConnection> tcp_conn_wkptr; 
    std::weak_ptr<Timer> timer_wkptr_; 
};