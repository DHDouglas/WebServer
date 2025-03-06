#pragma once 

#include <memory>
#include <functional>
#include <map>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "channel.h"
#include "eventloop.h"
#include "timestamp.h"
#include "acceptor.h"
#include "eventloop_thread.h"
#include "tcp_connection.h"

class Acceptor; 

class TcpConnection;

class TcpServer {
public:
    using Buffer = char; 
    using ThreadInitCallback = std::function<void(EventLoop*)>; 
    using ConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, Buffer*, size_t n)>; 
    using WriteCompleteCallback = ConnectionCallback;
    using CloseCallback = ConnectionCallback;
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>; 

public: 
    TcpServer(EventLoop* loop, const InetAddress& addr, const std::string& name = "");
    ~TcpServer(); 

    // Thread safe. Starts the server if it's not listening. Harmless to call it multiple times. 
    void start();  
    // Not thread safe.
    void setConnectionCallback(const ConnectionCallback& cb); 
    // Not thread safe.
    void setMessageCallback(const MessageCallback& cb); 

    // Not thread safe, but in loop.
    void newConnection(int sockfd, const InetAddress& addr);  
    // Thread safe.
    void removeConnection(const TcpConnectionPtr& conn); 
    // Not thread safe, but in loop.
    void removeConnectionInLoop(const TcpConnectionPtr& conn); 


private:
    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

private:
    EventLoop* loop_;   // the acceptor loop 

    const std::string ip_port_; 
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; 
    std::unique_ptr<EventLoopThread> eventloop_thread; 
    // std::shared_ptr<EventLoopThreadPool> eventloop_thread_pool_; 

    ConnectionCallback connCallback_;
    MessageCallback msgCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_; 
    std::atomic<bool> started_; 
    ConnectionMap connections_;
    size_t next_conn_id_; 
};