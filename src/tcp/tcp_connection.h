#pragma once 

#include <memory>
#include <cassert>
#include <functional>

#include "channel.h"
#include "timestamp.h"
#include "inet_address.h"
#include "buffer.h"


class EventLoop; 

class TcpConnection : public std::enable_shared_from_this<TcpConnection>{
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


    void setConnectionCallback(const ConnectionCallback& cb) {
        connCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback& cb) {
        msgCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }

    // 该回调用于通知TcpServer移除持有的指向该对象的TcpConnectionPtr, 非用户回调. 
    void setCloseCallback(const CloseCallback& cb) { 
        closeCallback_ = cb;
    }

    // called when TcpServer accepts a new connection. should be called only once
    void connectionEstablished(); 
    // called when TcpServer has removed the conn from its map. should be called only once
    void connectionDestroyed(); 


private:
    enum class State { kConnecting, kConnected, kDisconnected, kDisconnecting }; 
    void setState(State s) { state_ = s; }
    const char* stateToString() const;
    void handleRead(Timestamp receive_time);
    void handleWrite();
    void handleClose();
    void handleError(); 


public:



private:
    EventLoop* loop_; 
    std::string name_; 
    State state_; 
    bool reading_; 
    // int sockfd_;
    Channel channel_; 
    InetAddress local_addr_;
    InetAddress peer_addr_; 

    ConnectionCallback connCallback_;  
    MessageCallback msgCallback_;
    WriteCompleteCallback writeCompleteCallback_; 
    CloseCallback closeCallback_;   // 用于通知TcpServer移除持有的指向该对象的TcpConnectionPtr, 非用户回调. 

    Buffer input_buffer_; 
};