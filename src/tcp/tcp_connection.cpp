#include "tcp_connection.h"
#include "eventloop.h"
#include <cassert>


TcpConnection::TcpConnection() {

}

TcpConnection::~TcpConnection() {

}


void TcpConnection::handleRead() {

    ssize_t n = read(channel_->fd(), buf, sizeof buf); 
    if (n > 0) {
        msgCallback_(this, buf, n);
    } else if (n == 0) {  // EOF, 连接关闭
        handleClose();    
    } else {
        handleError();   
    }
}


void TcpConnection::handleClose() {
    loop_->assertInLoopThread(); 
    assert(state_ == kConnected);
    channel_->disableAll();  // 对fd的关闭留给channel的析构函数
    closeCallback_(this); 
}

void TcpConnection::handleError() {

}

void TcpConnection::connectDestroyed() {
    
}
