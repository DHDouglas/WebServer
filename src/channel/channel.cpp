#include "channel.h"
#include "eventloop.h"
#include <cassert>
#include <cstdlib>


Channel::Channel(EventLoop* loop, int fd) 
    : loop_(loop), 
    fd_(fd), 
    events_(0), 
    revents_(0),
    event_handling(false),
    state_(StateInEpoll::NOT_EXIST)
{

}

Channel::~Channel() {
    assert(!event_handling);
    assert(state_ == StateInEpoll::NOT_EXIST); 
    if (loop_->isInLoopThread()) {
        assert(!loop_->hasChannel(this)); 
    }
}

EventLoop* Channel::getOwnerLoop() const {
    return loop_;
}

int Channel::getFd() const {
    return fd_;
}

int Channel::getEvents() const {
    return events_;
}

void Channel::setRevents(int evt) {
    revents_ = evt; 
}

void Channel::handleEvents() {
    event_handling = true; 
    if (revents_ & EPOLLERR) {
        if (errorCallback_) errorCallback_();
    }

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {  // 对端完全关闭连接,例如本端收到RST
        if (closeCallback_) closeCallback_();  
    }

    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) { // 可读, 带外数据可读, 对端半关闭(可读EOF)
        if (readCallback_) readCallback_(); 
    }

    if (revents_ & EPOLLOUT) {  // 可写
        if (writeCallback_) writeCallback_(); 
    }
    event_handling = false; 
}


void Channel::enableReading() {
    events_ |= kReadEvent; 
    updateInEpoller(); 
}

void Channel::disableReading() {
    events_ &= ~kReadEvent; 
    updateInEpoller(); 
}

void Channel::enableWriting() {
    events_ |= kWriteEvent;
    updateInEpoller(); 
}

void Channel::disableWriting() {
    events_ &= ~kWriteEvent; 
    updateInEpoller(); 
}

void Channel::disableAll() {
    events_ = kNoneEvent;
    updateInEpoller(); 
}

bool Channel::isReading() const {
    return events_ & kReadEvent;
}

bool Channel::isWriting() const {
    return events_ & kWriteEvent;
}

bool Channel::isNoneEvent() const {
    return events_ == kNoneEvent; 
}


void Channel::updateInEpoller() {
    loop_->updateChannelInEpoller(this);
}

void Channel::removeFromEpoller() {
    assert(isNoneEvent()); 
    loop_->removeChannelFromEpoller(this); 
}

void Channel::setState(StateInEpoll state) {
    state_ = state; 
}

Channel::StateInEpoll Channel::getState() {
    return state_;
}

void Channel::setReadCallback(Callback cb) {
    readCallback_ = std::move(cb); 
}

void Channel::setWriteCallback(Callback cb) {
    writeCallback_ = std::move(cb); 
}

void Channel::setErrorCallback(Callback cb) {
    errorCallback_ = std::move(cb); 
}

void Channel::setCloseCallback(Callback cb) {
    closeCallback_ = std::move(cb);
}