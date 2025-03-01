#include "channel.h"
#include "eventloop.h"


Channel::Channel(EventLoop* loop, int fd) 
    : loop_(loop), fd_(fd), events_(0), revents_(0), idx_(-1) {}

Channel::~Channel() {}

void Channel::handleEvents() {
    if (revents_ & EPOLLERR) {
        if (errorCallBack_) errorCallBack_();
    }

    if (revents_ & EPOLLHUP) {  // 断开连接

    }

    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) { // 可读, 带外数据可读, 对端关闭连接(可读EOF)
        if (readCallBack_) readCallBack_(); 
    }

    if (revents_ & EPOLLOUT) {  // 可写
        if (writeCallBack_) writeCallBack_(); 
    }
    // 
}

void Channel::addToEpoller() {
    loop_->addToEpoller(this); 
}

void Channel::modInEpoller() {
    loop_->modInEpoller(this);
}

void Channel::delFromEpoller() {
    loop_->delInEpoller(this); 
}


int Channel::getFd() const {
    return fd_;
}

int Channel::getEvents() const {
    return events_;
}

void Channel::setEvents(int evt) {
    events_ = evt; 
}

void Channel::setRevents(int evt) {
    revents_ = evt; 
}

void Channel::setReadCallBack(CallBack cb) {
    readCallBack_ = std::move(cb); 
}

void Channel::setWriteCallBack(CallBack cb) {
    writeCallBack_ = std::move(cb); 
}

void Channel::setErrorCallBack(CallBack cb) {
    errorCallBack_ = std::move(cb); 
}