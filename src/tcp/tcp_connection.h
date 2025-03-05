#pragma once 

#include <memory>
#include <netinet/in.h>

#include "channel.h"
#include "timestamp.h"
class EventLoop; 

class TcpConnection {
public:
    using ConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using CloseCallback = ConnectionCallback;
    using WriteCompleteCallback = ConnectionCallback;
    using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, Buffer*, Timestamp)>; 

private:
    enum State { kConnecting, kConnected, kDisconnected, kDisconnecting }; 
    void setState(State s) { state_ = s; }
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError(); 
    void connectDestroyed(); 

public:



private:
    EventLoop* loop_; 
    size_t id_; 
    State state_; 
    int sockfd_;
    Channel channel_; 
    struct sockaddr_in local_addr;
    struct sockaddr_in peer_addr; 
    ConnectionCallback connCallback_; 
    MessageCallback msgCallback_;
    CloseCallback closeCallback_; 
};