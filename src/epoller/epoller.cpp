#include "epoller.h"
#include "eventloop.h"

static const int INIT_EVENT_LIST_SIZE = 4096; 

Epoller::Epoller(EventLoop* loop) : owner_loop_(loop), events_(INIT_EVENT_LIST_SIZE) {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE); 
    }

}

Epoller::~Epoller() {}

int Epoller::getEpollFd() const {
    return epoll_fd_; 
}

void Epoller::poll(int timeout_ms, ChannelList* activeChannels) {
    int nfds = epoll_wait(epoll_fd_, events_.data(), static_cast<int>(events_.size()),  timeout_ms); 
    if (nfds > 0) {
        fillActiveChannels(nfds, activeChannels); 
    }
    if (static_cast<size_t>(nfds) == events_.size()) {
        events_.resize(events_.size() * 2);   // 根据I/O繁忙程度扩容.
    }
}

void Epoller::fillActiveChannels(int nfds, ChannelList* activeChannels) const {
    // 根据nfds, 设置对应channel中的事件
    for (int i = 0; i < nfds; ++i) {
        // struct epoll_event结构中的`.data`结构里的ptr字段, 可用于为fd绑定用户自定义的关联数据.
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);  
        channel->setRevents(events_[i].events); 
        activeChannels->push_back(channel); 
    }
}


void Epoller::epollAdd(Channel* channel) {
    struct epoll_event ev;
    int fd = channel->getFd();
    ev.data.fd = fd; 
    ev.data.ptr = channel;  
    ev.events = channel->getEvents(); 
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        perror("epoll_add error");
        return; 
    }
    channels_[fd] = channel;
}

// 是由外部修改channel的监听事件后, 再调用epoll_mod修改吗? 为什么不直接传递fd, 交由
void Epoller::epollMod(Channel* channel) {
    struct epoll_event ev;
    int fd = channel->getFd();
    ev.data.fd = fd; 
    ev.data.ptr = channel;  
    ev.events = channel->getEvents(); 
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
        perror("epoll_mod error"); 
        return;
    }
    channels_[fd] = channel;
}

void Epoller::epollDel(Channel* channel) {
    int fd = channel->getFd(); 
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) < 0) {
        perror("epoll_del error"); 
    }
    size_t rec = channels_.erase(fd);
    assert(rec == 1);
}

void Epoller::assertInLoopThread() const {
    owner_loop_->assertInLoopThread(); 
}