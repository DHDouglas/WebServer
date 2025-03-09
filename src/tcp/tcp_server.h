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
#include "eventloop_threadpool.h"
#include "tcp_connection.h"

class Acceptor; 

class TcpConnection;

class Buffer;

class TcpServer {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; 
    using ConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, Buffer*, Timestamp)>; 
    using WriteCompleteCallback = ConnectionCallback;
    using CloseCallback = ConnectionCallback;
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>; 

public: 
    TcpServer(EventLoop* loop, const InetAddress& addr, const std::string& name = "");
    ~TcpServer(); 

    // Thread safe. Starts the server if it's not listening. Harmless to call it multiple times. 
    void start();  
    // Not thread safe.
    void setConnectionCallback(const ConnectionCallback& cb) { connCallback_ = cb;  }
    // Not thread safe.
    void setMessageCallback(const MessageCallback& cb) { msgCallback_ = cb; }

    // Not thread safe, but in loop.
    void newConnection(int sockfd, const InetAddress& addr);  
    // Thread safe.
    void removeConnection(const TcpConnectionPtr& conn); 
    // Not thread safe, but in loop.
    void removeConnectionInLoop(const TcpConnectionPtr& conn); 


    /* Set the number of threads for handling input. 
     *
     * Always accepts new connection in loop's thread.
     * Must be called before @c start
     * @param numThreads 
     * - 0 means all I/O in loop's thread, no thread will created. 
     *   this is the default value. 
     * - 1 means all I/O in another thread. 
     * - N means a thread pool with N threads, new connections 
     *   are assigned on a round-robin basis.
     */
    void setThreadNum(int num_threads);
    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
    /// valid after calling start()
    std::shared_ptr<EventLoopThreadPool> threadPool() { return eventloop_thread_pool_; }

    std::string getIpPort() { return ip_port_; }
    std::string getName() { return name_;  }

private:
    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

private:
    EventLoop* loop_;   // the acceptor loop 

    const std::string ip_port_; 
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; 
    std::shared_ptr<EventLoopThreadPool> eventloop_thread_pool_; 

    ConnectionCallback connCallback_;
    MessageCallback msgCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_; 
    std::atomic<bool> started_; 
    ConnectionMap connections_;
    size_t next_conn_id_; 
};