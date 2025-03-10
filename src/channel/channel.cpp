#include "channel.h"

#include <memory>
#include <sstream>
#include <cassert>

#include "eventloop.h"
#include "timestamp.h"
#include "logger.h"

using namespace std;

Channel::Channel(EventLoop* loop, int fd) 
    : loop_(loop), 
    fd_(fd), 
    events_(0), 
    revents_(0),
    event_handling(false),
    state_(StateInEpoll::NOT_EXIST),
    tied_(false)
{

}

Channel::~Channel() {
    assert(!event_handling);  // 事件处理期间不会析构. 
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

void Channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
}

void Channel::handleEvents(Timestamp receive_time) {
    std::shared_ptr<void> gurad;
    if (tied_) {
        // 若tie_对象仍存在, 则保证其在handleEvents执行期间存活, 不被销毁.
        gurad = tie_.lock();  
        if (gurad) handleEventWithGuard(receive_time); 
    } else {
        handleEventWithGuard(receive_time);
    }
}

void Channel::handleEventWithGuard(Timestamp receive_time) {
    event_handling = true; 
    LOG_TRACE << reventsToString(); 
    if (revents_ & EPOLLERR) {
        if (errorCallback_) errorCallback_();
    }

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {  // 对端完全关闭连接,例如本端收到RST
        if (closeCallback_) closeCallback_();  
    }

    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) { // 可读, 带外数据可读, 对端半关闭(可读EOF)
        if (readCallback_) readCallback_(receive_time); 
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

void Channel::setReadCallback(ReadCallback cb) {
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


string Channel::eventsToString(int fd, int ev) {
    ostringstream oss;
    oss << fd << ": ";
    if (ev & EPOLLIN) oss << "EPOLLIN ";
    if (ev & EPOLLPRI) oss << "EPOLLPRI ";
    if (ev & EPOLLOUT) oss << "EPOLLOUT ";
    if (ev & EPOLLHUP) oss << "EPOLLHUP ";
    if (ev & EPOLLRDHUP) oss << "EPOLLRDHUP ";
    if (ev & EPOLLERR) oss << "EPOLLERR ";
    return oss.str();
}

string Channel::eventsToString() const {
    return eventsToString(fd_, events_);
}

string Channel::reventsToString() const {
    return eventsToString(fd_, revents_);
}
