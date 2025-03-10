#include "tcp_server.h"

#include "eventloop.h"
#include "acceptor.h"
#include "logger.h"

using namespace std;

TcpServer::TcpServer(EventLoop* loop, const InetAddress& addr, const string& name)
    : loop_(loop),
    ip_port_(addr.getIpPortString()),
    name_(name),
    acceptor_(make_unique<Acceptor>(loop_, addr)),
    eventloop_thread_pool_(make_shared<EventLoopThreadPool>(loop_, name_)),
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
        eventloop_thread_pool_->start(threadInitCallback_); 
        assert(!acceptor_->isListening());  
        loop_->runInLoop(bind(&Acceptor::listen, acceptor_.get()));
    }
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

    EventLoop* io_loop = eventloop_thread_pool_->getNextLoop(); 
    auto conn = make_shared<TcpConnection>(io_loop, conn_name, sockfd, local_addr, peer_addr); 
    connections_[conn_name] = conn; 

    // conn的回调
    conn->setConnectionCallback(connCallback_);  // 用户回调
    conn->setMessageCallback(msgCallback_);      // 用户回调
    conn->setWriteCompleteCallback(writeCompleteCallback_); 
    conn->setCloseCallback(
        bind(&TcpServer::removeConnection, this, placeholders::_1));
    // 放到runInLoop, 保证移除TcpConnection的操作只能在其所属EventLoop所在线程中执行
    // 注: bind内部拷贝得到的是weak_ptr, 而不是shared_ptr. gbd跟踪显示weak count为2.
    io_loop->runInLoop(bind(&TcpConnection::connectionEstablished, conn)); 
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
    // 获取conn所属EventLoop, 放到该loop所在IO线程执行
    EventLoop* ioLoop = conn->getOwnerLoop();
    // 必须加到EventLoop的任务队列中作为PendingFunctor执行, 
    // 而不能作为channel_.handleEvent()中的一步被直接调用. 
    // 防止channel_.handleEvent() => 析构该channel本身.
    ioLoop->addToQueueInLoop(bind(&TcpConnection::connectionDestroyed, conn));
}


void TcpServer::setThreadNum(int num_threads) {
    assert(num_threads >= 0);
    eventloop_thread_pool_->setThreadNum(num_threads);
}
