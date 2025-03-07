#include "tcp_server.h"
#include "acceptor.h"
#include "tcp_connection.h"
#include <atomic>

using namespace std;

TcpServer::TcpServer(EventLoop* loop, const InetAddress& addr, const string& name)
    : loop_(loop),
    ip_port_(addr.getIpString()),
    name_(name),
    acceptor_(make_unique<Acceptor>(loop_, addr)),
    started_(false), 
    next_conn_id_(1)
{
    acceptor_->setNewConnCallback(
        bind(&TcpServer::newConnection, this, placeholders::_1, placeholders::_2));
}



TcpServer::~TcpServer() {
    loop_->assertInLoopThread(); 
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";
    // 销毁其持有的所有TcpConnection. 
    for (auto& item : connections_) {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getOwnerLoop()->runInLoop(
            std::bind(&TcpConnection::connectionDestroyed, conn));
    }
}

void TcpServer::start() {
    if (!started_.exchange(true, std::memory_order_acq_rel)) {
        assert(!acceptor_->isListening());  
        loop_->runInLoop(bind(&Acceptor::listen, acceptor_.get()));
    }
}


void TcpServer::setConnectionCallback(const ConnectionCallback& cb) {
    connCallback_ = cb; 
}

void TcpServer::setMessageCallback(const MessageCallback& cb) {
    msgCallback_ = cb;
}



// 为conn_fd绑定一个TcpConnection对象. 
void TcpServer::newConnection(int sockfd, const InetAddress& peer_addr) {
    loop_->assertInLoopThread(); 

    int conn_id = next_conn_id_++;
    string conn_name = name_ + "#" + to_string(conn_id); 
    LOG_INFO << "TcpServer::newConnection [" << name_
    << "] - new connection [" << conn_name 
    << "] from " << peer_addr.getIpPortString(); 

    InetAddress local_addr(InetAddress::getLocalAddrBySockfd(sockfd)); 
    // FIXME poll with zero timeout to double confirm the new connection
    auto conn = make_shared<TcpConnection>(loop_, conn_name, sockfd, local_addr, peer_addr); 
    connections_[conn_name] = conn; 

    // conn的回调
    conn->setConnectionCallback(connCallback_);
    conn->setMessageCallback(msgCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_); 
    conn->setCloseCallback(
        bind(&TcpServer::removeConnection, this, placeholders::_1));
    // 放到runInLoop, 保证移除TcpConnection的操作只能在其所属EventLoop所在线程中执行
    // 注: bind内部拷贝得到的是weak_ptr, 而不是shared_ptr. gbd跟踪显示weak count为2.
    loop_->runInLoop(bind(&TcpConnection::connectionEstablished, conn)); 
}


void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    // 放到runInLoop, 保证移除TcpConnection的操作只能在其所属EventLoop所在线程中执行
    loop_->runInLoop(bind(&TcpServer::removeConnectionInLoop, this, conn)); 
}


void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    loop_->assertInLoopThread();
    string conn_name = conn->getName();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn_name;

    size_t n = connections_.erase(conn_name);
    assert(n == 1); 
    EventLoop* ioLoop = conn->getOwnerLoop();
    // 必须加到EventLoop的任务队列中作为PendingFunctor执行, 
    // 而不能作为channel_.handleEvent()中的一步被直接调用. 
    // 防止channel_.handleEvent() => 析构该channel本身.
    ioLoop->addToQueueInLoop(bind(&TcpConnection::connectionDestroyed, conn));
}