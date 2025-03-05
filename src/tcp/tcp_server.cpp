#include "tcp_server.h"

TcpServer::TcpServer(EventLoop* loop, const struct sockaddr_in& addr)
    : loop_(loop)
{

}


void TcpServer::setConnectionCallback(const ConnectionCallback& cb) {
    connCallback_ = cb; 
}

void TcpServer::setMessageCallback(const MessageCallback& cb) {
    msgCallback_ = cb;
}



// 为conn_fd绑定一个TcpConnection对象. 
void TcpServer::newConnection(int sockfd, const struct sockaddr_in& addr) {
    loop_->assertInLoopThread(); 
    int conn_id = next_conn_id_++;

    // TODO: 根据sockfd, 获取对端地址, 本端地址.
    auto conn = make_shared<TcpConnection>(loop_, conn_id, sockfd, localAddr, peerAddr); 
    connections_[conn_id] = conn; 
    // TODO: 为conn设置回调函数.

    
}