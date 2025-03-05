#pragma once 

#include <memory>
#include <functional>
#include <map>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>

#include "channel.h"
#include "timestamp.h"
#include "acceptor.h"
#include "eventloop_thread.h"
#include "tcp_connection.h"

class TcpConnection;

class TcpServer {
public:
    using ConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using CloseCallback = ConnectionCallback;
    using WriteCompleteCallback = ConnectionCallback;
    using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, Buffer*, Timestamp)>; 
    using ConnectionMap = std::map<size_t, std::shared_ptr<TcpConnection>>;

public: 
    TcpServer(EventLoop* loop, const struct sockaddr_in& addr);
    ~TcpServer(); 

    void start(); 
    void setConnectionCallback(const ConnectionCallback& cb); 
    void setMessageCallback(const MessageCallback& cb); 

    void newConnection(int sockfd, const struct sockaddr_in& addr); 


private:
    EventLoop* loop_;   // the acceptor loop 

    const int port_; 
    // const std::string name_, 

    std::unique_ptr<Acceptor> acceptor_; 
    std::shared_ptr<EventLoopThread> eventloop_thread_pool_; 

    ConnectionCallback connCallback_;
    MessageCallback msgCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    std::atomic<bool> started_; 
    ConnectionMap connections_;
    size_t next_conn_id_; 
};