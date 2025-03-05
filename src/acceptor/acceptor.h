#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <string>
#include <functional>
#include <cstdio>
#include <cstdlib>

#include "eventloop.h"
#include "channel.h"

class Acceptor{ 
public:
    using NewConnectionCallBack = std::function<void(int connfd, struct sockaddr_in& addr)>; 
    Acceptor(EventLoop* loop, int port);
    ~Acceptor(); 

public:
    void setNewConnCallBack(const NewConnectionCallBack& cb); 
    int createSocketAndBind(); 
    void listen(); 
    bool ifListening() const; 
    void handleAccept(); 
    
private:
    EventLoop* loop_; 
    std::string ip_; 
    int port_;
    struct sockaddr_in addr; 
    int addr_len; 
    int sockfd_; 
    Channel accept_channel_; 
    NewConnectionCallBack newConnCallBack_; 
    std::atomic<bool> listening_; 
};