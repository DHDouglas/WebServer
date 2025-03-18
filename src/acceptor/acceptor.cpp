#include "acceptor.h"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "eventloop.h"
#include "logger.h"

using namespace std;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listen_addr)
    : loop_(loop),
    addr_(listen_addr), 
    sockfd_(createSocketAndBind()),
    accept_channel_(loop, sockfd_),
    listening_(false),
    idlefd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    accept_channel_.setReadCallback(bind(&Acceptor::handleAccept, this));
}

Acceptor::~Acceptor() {
    accept_channel_.disableAll();
    accept_channel_.removeFromEpoller();
    ::close(sockfd_); 
}

void Acceptor::setNewConnCallback(const NewConnectionCallBack& cb) {
    newConnCallBack_ = cb; 
}

bool Acceptor::isListening() const {
    return listening_; 
}

int Acceptor::createSocketAndBind() {
    // 设置地址, 端口号等等属性. 
    int listen_fd = socket(addr_.getFamily(), SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0); 
    if (listen_fd == -1) {
        LOG_SYSFATAL << "Acceptor::createSocketAndBind() socket"; 
    }

    int optval = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        LOG_SYSFATAL << "Acceptor::createSocketAndBind() setsockopt"; 
    }

    if (bind(listen_fd, addr_.getSockAddr(), addr_.getAddrLen()) < 0) {
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
    InetAddress peer_addr; 
    struct sockaddr_in6 sockaddr6; 
    socklen_t addr6len = static_cast<socklen_t>(sizeof(sockaddr6)); 
    memset(&sockaddr6, 0, addr6len); 
    int connfd = ::accept4(sockfd_, InetAddress::sockAddrCast(&sockaddr6),
        &addr6len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) {
        if (newConnCallBack_) {
            peer_addr.setSockAddrInet6(sockaddr6); 
            newConnCallBack_(connfd, peer_addr);  // 执行新连接的回调
        } else {
            ::close(connfd); 
        }
    } else {
        LOG_SYSERR << "Acceptor::handleAccept() accept";
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
            LOG_FATAL << "unexpected error of ::accept " << saved_errno;
            break; 
          default:
            LOG_FATAL << "unknown error of ::accept " << saved_errno;
            break;
        }
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        if (errno == EMFILE)
        {
          ::close(idlefd_);
          idlefd_ = ::accept(sockfd_, NULL, NULL);
          ::close(idlefd_);
          idlefd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
