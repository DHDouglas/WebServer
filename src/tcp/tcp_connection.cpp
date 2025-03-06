#include "tcp_connection.h"
#include "eventloop.h"
#include "logger.h"

using namespace std;

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& name, 
                             int sockfd,
                             const InetAddress& local_addr,
                             const InetAddress& peer_addr)
    : loop_(loop),
    name_(name),
    state_(State::kConnecting),
    reading_(false), 
    // sockfd_(sockfd),
    channel_(loop, sockfd),
    local_addr_(local_addr),
    peer_addr_(peer_addr)
{
    channel_.setReadCallback(
        bind(&TcpConnection::handleRead, this));
    channel_.setWriteCallback(
        bind(&TcpConnection::handleWrite, this));
    channel_.setCloseCallback(
        bind(&TcpConnection::handleClose, this));
    channel_.setErrorCallback(
        bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::construct[" <<  name_  << "] at " << this
              << " fd=" << sockfd;

    // Tcp keepAlive
    // int optval = 1; 
    // ::setsockopt(channel_.getFd(), SOL_SOCKET, SO_KEEPALIVE,
    //              &optval, static_cast<socklen_t>(sizeof optval));
}   


TcpConnection::~TcpConnection() {
    LOG_DEBUG << "TcpConnection::destroyed[" <<  name_ << "] at " << this
              << " fd=" << channel_.getFd()
              << " state=" << stateToString();
    assert(state_ == State::kDisconnected);
    ::close(channel_.getFd());  
}


void TcpConnection::handleRead() {
    LOG_TRACE << "TcpConnection::handleRead() msgCallback_ invoked"; 
    char buf[65536]; 
    ssize_t n = read(channel_.getFd(), buf, sizeof buf); 
    if (n > 0) {
        msgCallback_(shared_from_this(), buf, n);
        LOG_TRACE << "TcpConnection::handleRead() msgCallback_ invoked"; 
    } else if (n == 0) {  // EOF, 连接关闭
        handleClose();    
    } else {
        handleError();   
    }
}

void TcpConnection::handleWrite() {

}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread(); 
    LOG_TRACE << "fd = " << channel_.getFd() << " state = " << stateToString();
    assert(state_ == State::kConnected || state_ == State::kDisconnecting);
    setState(State::kDisconnected); 
    channel_.disableAll();           // 对fd的关闭留给channel的析构函数

    TcpConnectionPtr guard_this(shared_from_this()); 
    connCallback_(guard_this);   // 用户回调
    closeCallback_(guard_this);  // 用于通知TcpServer移除持有的指向该对象的TcpConnectionPtr, 非用户回调. 
}

void TcpConnection::handleError() {
    int err; 
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    if (::getsockopt(channel_.getFd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }
    LOG_ERROR << "TcpConnection::handleError [" 
              << name_<< "] - SO_ERROR = " << err << " " << strerror_tl(err);

}

void TcpConnection::connectionEstablished() {
    loop_->assertInLoopThread(); 
    assert(state_ == State::kConnecting);
    setState(State::kConnected);

    // 将该TcpConnection对象绑定到channel_
    // 确保channel_执行handleEvent()时该TcpConnection对象不会被析构
    channel_.tie(shared_from_this());  
    channel_.enableReading(); 
    // 执行用户回调.
    connCallback_(shared_from_this()); 
}


void TcpConnection::connectionDestroyed() {
    loop_->assertInLoopThread(); 
    if (state_ == State::kConnected) {
        setState(State::kDisconnected);
        channel_.disableAll(); 
        connCallback_(shared_from_this());  // 执行用户回调. 
    }
    channel_.removeFromEpoller(); 
}



const char* TcpConnection::stateToString() const {
    switch (state_) {
      case State::kDisconnected:
        return "kDisconnected";
      case State::kConnecting:
        return "kConnecting";
      case State::kConnected:
        return "kConnected";
      case State::kDisconnecting:
        return "kDisconnecting";
      default:
        return "unknown state";
    }
}
