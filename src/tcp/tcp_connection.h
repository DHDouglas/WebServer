#pragma once 

#include <memory>
#include <functional>

#include "channel.h"
#include "timestamp.h"
#include "inet_address.h"
#include "buffer.h"
#include "any.h"


class EventLoop; 

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using ConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using CloseCallback = ConnectionCallback;
    using WriteCompleteCallback = ConnectionCallback;
    using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, Buffer*, Timestamp t)>; 
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>; 

public:
    TcpConnection(EventLoop* loop, 
                  const std::string& name, 
                  int sockfd, 
                  const InetAddress& local_addr, 
                  const InetAddress& peer_addr);
    ~TcpConnection(); 

    bool connected() const { return state_ == State::kConnected; }
    bool disconnected() const { return state_ == State::kDisconnected; }

    EventLoop* getOwnerLoop() const { return loop_; }
    std::string getName() const { return name_; }
    const InetAddress& getLocalAddress() const { return local_addr_; }
    const InetAddress& getPeerAddress() const { return peer_addr_; }

    // 用户回调
    void setConnectionCallback(const ConnectionCallback& cb) { connCallback_ = cb;}
    void setMessageCallback(const MessageCallback& cb) { msgCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }

    // 非用户回调. 该回调用于通知TcpServer移除持有的指向该对象的TcpConnectionPtr.
    void setCloseCallback(const CloseCallback& cb) {  closeCallback_ = cb; }

    // 供TcpServer调用
    // called when TcpServer accepts a new connection. should be called only once
    void connectionEstablished(); 
    // called when TcpServer has removed the conn from its map. should be called only once
    void connectionDestroyed(); 

    void send(const void* message, size_t len); 
    void send(const std::string& message); 
    void send(Buffer* message);
    void send(struct iovec vecs[], size_t iovcnt);


    // 半关闭, 仅关闭写端
    void shutdown(); // 调用shutdownInLoop, 后者保证在EventLoop所属的IO线程调用
    void forceClose(); 

    void setContext(const Any& context) { context_ = context; }
    Any* getMutableContext() { return &context_; }

private:
    // - kConnecting: 调用connectionEstablished()之前
    // - kkConnected: connectionEstablished()调用后.
    // - kDisconnecting: 调用shutdown关闭写端之后, 半关闭状态
    // - kDisconnected: handleClose()执行后, 全关闭状态.
    enum class State { kConnecting, kConnected, kDisconnected, kDisconnecting }; 
    void setState(State s) { state_ = s; }
    const char* stateToString() const;
    void handleRead(Timestamp receive_time);
    void handleWrite();
    void handleClose();
    void handleError(); 

    void sendInLoop(const void* message, size_t len); 
    void sendInLoop(struct iovec vecs[], size_t iovcnt);
    void shutdownInLoop();  
    void forceCloseInLoop();
    
    // 以vector<char>形式拷贝数据副本.
    std::vector<char> persistData(const void* data, size_t len);
    std::vector<char> persistData(struct iovec vecs[], size_t iovcnt);



private:
    EventLoop* loop_; 
    std::string name_; 
    State state_; 
    bool reading_; 
    Channel channel_; 
    InetAddress local_addr_;
    InetAddress peer_addr_; 
    Any context_;

    ConnectionCallback connCallback_;  
    MessageCallback msgCallback_;
    WriteCompleteCallback writeCompleteCallback_; 
    CloseCallback closeCallback_;   // 用于通知TcpServer移除持有的指向该对象的TcpConnectionPtr, 非用户回调. 

    Buffer input_buffer_; 
    Buffer output_buffer_; 
};