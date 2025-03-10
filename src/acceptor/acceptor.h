#pragma once

#include <functional>
#include <atomic>

#include "channel.h"
#include "inet_address.h"

class EventLoop;

class Acceptor{ 
public:
    using NewConnectionCallBack = std::function<void(int connfd, const InetAddress& addr)>; 
    Acceptor(EventLoop* loop, const InetAddress& listen_addr);
    ~Acceptor(); 

public:
    void setNewConnCallback(const NewConnectionCallBack& cb); 
    int createSocketAndBind(); 
    void listen(); 
    bool isListening() const; 
    void handleAccept(); 
    
private:
    EventLoop* loop_; 
    InetAddress addr_; 
    int sockfd_; 
    Channel accept_channel_; 
    NewConnectionCallBack newConnCallBack_; 
    std::atomic<bool> listening_; 
};