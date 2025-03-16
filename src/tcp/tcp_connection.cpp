#include "tcp_connection.h"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <unistd.h>
#include <cassert>
#include <sys/uio.h>

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
        bind(&TcpConnection::handleRead, this, placeholders::_1));
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


void TcpConnection::handleRead(Timestamp receive_time) {
    LOG_TRACE << "TcpConnection::handleRead() msgCallback_ invoked"; 
    int save_errno = 0;
    ssize_t n = input_buffer_.readFd(channel_.getFd(), &save_errno); 
    if (n > 0) {
        msgCallback_(shared_from_this(), &input_buffer_, receive_time);
        LOG_TRACE << "TcpConnection::handleRead() msgCallback_ invoked"; 
    } else if (n == 0) {  // EOF, 连接关闭
        handleClose();    
    } else {
        errno = save_errno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();   
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread(); 
    if (channel_.isWriting()) {
        // OutputBuffer => fd;
        ssize_t n = ::write(channel_.getFd(), 
                            output_buffer_.peek(), 
                            output_buffer_.readableBytes()); 
        if (n > 0) {
            LOG_TRACE << "TcpConnection::handleWrite send " << n << "bytes"; 
            output_buffer_.retrieve(n);
            if (output_buffer_.readableBytes() == 0) {  // TCP发送缓冲区清空, 数据发送完毕, 取消监听EPOLLOUT.
                channel_.disableWriting(); 
                if (writeCompleteCallback_) {
                    loop_->addToQueueInLoop(bind(writeCompleteCallback_, shared_from_this())); 
                }
                if (state_ == State::kDisconnecting) {  // 正在断开连接的过程中, 关闭写端.
                    shutdownInLoop(); 
                }
            }
        } else {
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    } else {
        LOG_TRACE << "Connection fd = " << channel_.getFd()
                  << " is down, no more writing";
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread(); 
    LOG_TRACE << "fd = " << channel_.getFd() << " state = " << stateToString();
    assert(state_ == State::kConnected || state_ == State::kDisconnecting);
    setState(State::kDisconnected); 
    channel_.disableAll();           // 对fd的关闭留给channel的析构函数

    TcpConnectionPtr guard_this(shared_from_this()); 
    connCallback_(guard_this);   // 用户回调
    closeCallback_(guard_this);  // 非用户回调, 为TcpServer注册的removeConnection回调. 作用:
                                 // 1) 告知TcpServer移除持有的指向该对象的TcpConnectionPtr.
                                 // 2) 由TcpServer调用TcpConnection::connectionEstablished(). 
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

void TcpConnection::shutdown() {
    if (state_ == State::kConnected) {
        setState(State::kDisconnecting); 
        loop_->runInLoop(bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_.isWriting()) {  // 关闭写端
        if (::shutdown(channel_.getFd(), SHUT_WR) < 0) {
          LOG_SYSERR << "sockets::shutdownWrite";
        }
    }
}

void TcpConnection::forceClose() {
    if (state_ == State::kConnected || state_ == State::kDisconnecting) {
        setState(State::kDisconnecting); 
        // 必须addToQueue作为Pendingfunctor执行, 避免handleEvent时直接析构TcpConnection, 导致channel也无了. 
        loop_->addToQueueInLoop(bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if (state_ == State::kConnected || state_ == State::kDisconnecting) {
        handleClose();
    }
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


void TcpConnection::send(const std::string& message) {
    send(message.c_str(), message.length()) ;
}

void TcpConnection::send(Buffer* buf) {
    send(buf->peek(), buf->readableBytes()); 
    // send()保证, 若数据不能立即发送或单次发送不完全部数据, 会进行拷贝或加入到Tcp发送缓冲区. 
    // 因此这里可以直接清除应用层缓冲. 
    buf->retrieveAll(); 
}

// const char*可以隐式转换为const void*, 无需单独重载. 
void TcpConnection::send(const void* data, size_t len) {
    if (state_ == State::kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(data, len); 
        } else {
            // 当前线程非所属EventLoop所在的IO线程, 故需作为PendingFunctor添加到后者的任务队列中. 
            // 由于不是立即执行, 因此必须赋值一份data, 避免在Loop中真正运行该函数时外部data指针已失效. 
            // 用vector<char>而不是string, 保证传输二进制数据, 不因为'\0'而截断.
            auto message = persistData(data, len); 
            loop_->runInLoop(
                [this, msg = std::move(message)]() { // 移动捕获. 
                    this->sendInLoop(msg.data(), msg.size());
                }
            );  
        }
    }
}

void TcpConnection::sendInLoop(const void* data, size_t len) {
    loop_->assertInLoopThread(); 
    ssize_t n_written = 0;
    size_t remaining = len;
    bool fault_error = false; 
    if (state_ == State::kDisconnected) {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    // 两种情况:
    // 1) 如果channel空闲且输出缓冲区为空, 即没有尚未发送出去的预留数据, 则直接向fd写, 不会导致数据乱序
    //    从而省略OutputBuffer->fd的过程(监听EPOLLOUT, 等待可写, 拷贝开销等)
    // 2) 否则, 将数据追加到OutputBuffer,  等到handleWrite()里进行写. 
    if (!channel_.isWriting() && output_buffer_.readableBytes() == 0) {
        n_written = ::write(channel_.getFd(), data, len); 
        if (n_written >= 0) {
            remaining -= n_written; 
            if (remaining == 0 && writeCompleteCallback_) { // 已全写完, 执行用户回调.
                loop_->addToQueueInLoop(bind(writeCompleteCallback_, shared_from_this())); 
            }
        } else {
            n_written = 0;
            if (errno != EWOULDBLOCK) {  // 非缓冲区已满
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) {  // 连接异常
                    fault_error = true; 
                }
            }
        }
    }
    LOG_TRACE << "TcpConnection::send send " << n_written 
              << " bytes, remaining: " << remaining; 
    assert(remaining <= len); 
    // 还有剩余数据
    if (!fault_error && remaining > 0) {  
        output_buffer_.append(static_cast<const char*>(data) + n_written, remaining); 
        if (!channel_.isWriting()) {   
            channel_.enableWriting();  // epoll上开启监听EPOLLOUT.
        }
    }
}


void TcpConnection::send(struct iovec vecs[], size_t iovcnt) {
    if (state_ == State::kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(vecs, iovcnt); 
        } else {
            // 当前线程非所属EventLoop所在的IO线程, 故需作为PendingFunctor添加到后者的任务队列中. 
            // 由于不是立即执行, 因此必须赋值一份data, 避免在Loop中真正运行该函数时外部data指针已失效. 
            // 用vector<char>而不是string, 保证传输二进制数据, 不因为'\0'而截断.
            auto message = persistData(vecs, iovcnt); 
            loop_->runInLoop(
                [this, msg = std::move(message)]() { // 移动捕获. 
                    this->sendInLoop(msg.data(), msg.size());
                }
            );  
        }
    }
}


void TcpConnection::sendInLoop(struct iovec vecs[], size_t iovcnt) {
    ssize_t n_written = 0;
    size_t total = 0;
    for (size_t i = 0; i < iovcnt; ++i) {
        total += vecs[i].iov_len;
    }
    size_t remaining = total; 

    bool fault_error = false;
    if (state_ == State::kDisconnected) {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    if (!channel_.isWriting() && output_buffer_.readableBytes() == 0) {
        n_written = ::writev(channel_.getFd(), vecs, iovcnt); 
        if (n_written >= 0) {
            remaining -= n_written;
            if (remaining == 0 && writeCompleteCallback_) { // 已全写完, 执行用户回调.
                loop_->addToQueueInLoop(bind(writeCompleteCallback_, shared_from_this())); 
            }
        } else {
            n_written = 0; 
            if (errno != EWOULDBLOCK) {
                LOG_SYSERR << "TcpConnection::sendInLoop writev";
                if (errno == EPIPE || errno == ECONNRESET) {
                    fault_error = true; 
                }
            }
        }
    }
    LOG_TRACE << "TcpConnection::send send " << n_written 
              << " bytes, remaining: " << remaining; 
    assert(remaining <= total); 
    // 对剩余数据写入发送缓冲区.
    if (!fault_error && remaining > 0) {
        size_t n = n_written; 
        size_t i = 0; 
        vector<iovec> rest;
        for (; i < iovcnt; ++i) {
            if (n >= vecs[i].iov_len) {
                n -= vecs[i].iov_len;
            } else {
                struct iovec vec;
                vec.iov_base = static_cast<char*>(vecs[i].iov_base) + n;
                vec.iov_len = vecs[i].iov_len - n;
                rest.push_back(std::move(vec));
                ++i;
                break;
            }
        }
        for (; i < iovcnt; ++i) {
            rest.push_back(vecs[i]);
        }
        output_buffer_.append(rest.data(), rest.size());
        if (!channel_.isWriting()) {
            channel_.enableWriting();  
        }
    }
}


vector<char> TcpConnection::persistData(const void* data, size_t len) {
    return vector<char>(
        static_cast<const char*>(data), 
        static_cast<const char*>(data) + len
    );
}

vector<char> TcpConnection::persistData(struct iovec vectors[], size_t iovcnt) {
    size_t total = 0;
    for (size_t i = 0; i < iovcnt; ++i) {
        total += vectors[i].iov_len;
    }
    vector<char> data(total);
    // char* dst = data.data(); 
    for (size_t i = 0; i < iovcnt; ++i) {
        // memcpy(dst, vectors[i].iov_base, vectors[i].iov_len);
        // dst += vectors[i].iov_len;
        data.insert(
            data.end(), 
            static_cast<const char*>(vectors[i].iov_base), 
            static_cast<const char*>(vectors[i].iov_base) + vectors[i].iov_len
        );

    };
    return data;
}
