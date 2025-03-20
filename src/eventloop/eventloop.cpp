#include "eventloop.h"

#include <sys/eventfd.h>
#include <signal.h>
#include <cassert>

#include "thread.h"
#include "channel.h"
#include "logger.h"
#include "epoller.h"

using namespace std;

thread_local EventLoop* EventLoop::t_loopInThisThread = nullptr; 

static constexpr int kPollTimeMs = 10000;


// 当向断开的conn_fd调用write写入时会收到SIGPIPE信号, 其默认行为是终止进程.
// 因此对于服务器端程序通常需要忽略该信号. 通过一个全局变量来实现. 
class IgnoreSigPipe {
public:
    IgnoreSigPipe() { ::signal(SIGPIPE, SIG_IGN);}
};

IgnoreSigPipe initObj;


EventLoop::EventLoop() 
    : looping_(false), 
    quit_(false),
    calling_pending_functors_(false),
    threadId_(CurrentThread::getTid()),
    epoller_(make_unique<Epoller>(this)), 
    timer_manager_(make_unique<TimerManager>(this)),
    wakeup_fd_(createEventfd()),
    wakeup_channel_(this, wakeup_fd_)
{
    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
    if (t_loopInThisThread) {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << threadId_;
    } else {
        t_loopInThisThread = this;
    }
    wakeup_channel_.setReadCallback(bind(&EventLoop::handleWakeUp, this));
    wakeup_channel_.enableReading(); 
}


EventLoop::~EventLoop() {
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_ 
              << " destructs in thread " << CurrentThread::getTid();
    assert(!looping_); 
    wakeup_channel_.disableAll(); 
    wakeup_channel_.removeFromEpoller(); 
    ::close(wakeup_fd_);  
    t_loopInThisThread = nullptr; // 保证当前线程可再持有EventLoop实例.
}


bool EventLoop::isInLoopThread() {
    return threadId_ == CurrentThread::getTid(); 
}

void EventLoop::abortNotInLoopThread() {
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::getTid();
}

void EventLoop::assertInLoopThread() {
    if (!isInLoopThread()) {
        abortNotInLoopThread(); 
    }
}

void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) {  // 若是在其他IO线程调用该quit, 必须保证唤醒.
        wakeup();  
    }
}

void EventLoop::loop() {
    assert(!looping_); 
    assertInLoopThread(); 
    looping_ = true;
    quit_ = false; 
    LOG_TRACE << "EventLoop " << this << " start looping";

    while (!quit_) {
        active_channels_.clear(); 
        poll_return_time_ = epoller_->poll(kPollTimeMs, &active_channels_); 
        event_handling_ = true;
        for (auto& channel : active_channels_) {
            current_active_channel_ = channel; 
            current_active_channel_->handleEvents(poll_return_time_); 
        }
        current_active_channel_ = nullptr; 
        event_handling_ = false; 
        doPendingFunctors();   
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false; 
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors; 
    calling_pending_functors_ = true; 
    {
        lock_guard<mutex> lock(mtx_); 
        functors.swap(pending_functors_);  // 直接快速交换.
    }
    for (const auto& functor : functors) {
        functor(); 
    }
    calling_pending_functors_ = false; 
}



void EventLoop::runInLoop(const Functor& cb) {
    if (isInLoopThread()) {
        cb(); 
    } else {
        addToQueueInLoop(cb); 
    }
}

void EventLoop::addToQueueInLoop(const Functor& cb) {
    {
        lock_guard<mutex> lock(mtx_);
        pending_functors_.push_back(cb); 
    }
    if (!isInLoopThread() || calling_pending_functors_) {
        wakeup(); 
    }
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeup_fd_, &one, sizeof(one)); 
    if (n != sizeof(one)) {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleWakeUp() {
    uint64_t one = 1;
    ssize_t n = read(wakeup_fd_, &one, sizeof one); 
    if (n != sizeof one) {
        LOG_ERROR << "EventLoop::handleWakeUp() reads " << n << " bytes instead of 8";
    }
}


bool EventLoop::hasChannel(Channel* channel) {
    assert(channel->getOwnerLoop() == this);
    assertInLoopThread(); 
    return epoller_->hasChannel(channel); 
}

void EventLoop::updateChannelInEpoller(Channel* channel) {
    assert(channel->getOwnerLoop() == this); 
    assertInLoopThread(); 
    epoller_->updateChannel(channel); 
}


void EventLoop::removeChannelFromEpoller(Channel* channel) {
    assert(channel->getOwnerLoop() == this); 
    assertInLoopThread(); 
    epoller_->removeChannel(channel); 
}


weak_ptr<Timer> EventLoop::runAt(const Timestamp& time, const TimerCallback& cb) {
    return timer_manager_->addTimer(cb, time, Duration(0)); 
}

weak_ptr<Timer> EventLoop::runAfter(const Duration& delay, const TimerCallback& cb) {
    auto now = Timestamp::now(); 
    return runAt(now + delay, cb);
}

weak_ptr<Timer> EventLoop::runEvery(const Duration& interval, const TimerCallback& cb) {
    return timer_manager_->addTimer(cb, Timestamp::now() + interval, interval); 
}

void EventLoop::removeTimer(const weak_ptr<Timer>& wk_ptr_timer) {
    timer_manager_->removeTimer(wk_ptr_timer); 
}

int EventLoop::createEventfd() {
    int efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); 
    if (efd < 0) {
        LOG_SYSFATAL << "Failed in eventfd";
    }
    return efd;
}

