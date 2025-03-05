#include "acceptor.h"


using namespace std;

Acceptor::Acceptor(EventLoop* loop, int port)
    : loop_(loop),
    port_(port), 
    addr_len(sizeof(addr)),
    sockfd_(createSocketAndBind()),
    accept_channel_(loop, sockfd_),
    listening_(false)
{
    accept_channel_.setReadCallback(bind(&Acceptor::handleAccept, this));
}

Acceptor::~Acceptor() {
    accept_channel_.disableAll();
    accept_channel_.removeFromEpoller();
    close(sockfd_); 
}

void Acceptor::setNewConnCallBack(const NewConnectionCallBack& cb) {
    newConnCallBack_ = cb; 
}

bool Acceptor::ifListening() const {
    return listening_; 
}

int Acceptor::createSocketAndBind() {
    // 设置地址, 端口号等等属性. 
    int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0); 
    if (listen_fd == -1) {
        LOG_SYSFATAL << "Acceptor::createSocketAndBind() socket"; 
    }

    int optval = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        LOG_SYSFATAL << "Acceptor::createSocketAndBind() setsockopt"; 
    }

    memset(&addr, 0, addr_len); 
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    addr.sin_port = htons(port_); 
    if (bind(listen_fd, (struct sockaddr*)&addr, addr_len) < 0) {
        close(listen_fd); 
        LOG_SYSFATAL << "Acceptor::createSocketAndBind() bind"; 
    }
    return listen_fd;
}

void Acceptor::listen() {
    loop_->assertInLoopThread(); 
    listening_ = true; 
    if (::listen(sockfd_, SOMAXCONN) < 0) {
        LOG_SYSFATAL << "Acceptor::listen() listen"; 
    }
    accept_channel_.enableReading(); 
}


void Acceptor::handleAccept() {
    // 处理连接
    loop_->assertInLoopThread();
    int connfd = ::accept4(sockfd_, (struct sockaddr*)&addr,
        (socklen_t*)&addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);     
    if (connfd >= 0) {
        if (newConnCallBack_) {
            newConnCallBack_(connfd, addr);  // 执行新连接的回调
        } else {
            close(connfd); 
        }
    } else {
        int saved_errno = errno;
        switch (saved_errno)
        {
          case EAGAIN:
          case ECONNABORTED:
          case EINTR:
          case EPROTO:
          case EPERM:
          case EMFILE: // per-process lmit of open file desctiptor ???
            // expected errors
            errno = saved_errno;
            break;
          case EBADF:
          case EFAULT:
          case EINVAL:
          case ENFILE:
          case ENOBUFS:
          case ENOMEM:
          case ENOTSOCK:
          case EOPNOTSUPP:
            // unexpected errors
            LOG_SYSFATAL << "unexpected error of ::accept " << saved_errno;
            break; 
          default:
            LOG_SYSFATAL << "unknown error of ::accept " << saved_errno;
            break;
        }
        LOG_SYSFATAL << "Acceptor::handleAccept() accept"; 
    }
}
