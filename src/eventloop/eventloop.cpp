#include "eventloop.h"
#include "channel.h"
#include "timer.h"
#include <sys/eventfd.h>

using namespace std;

thread_local EventLoop* EventLoop::loopInThisThread = nullptr; 

static constexpr int kPollTimeMs = 10000;

EventLoop::EventLoop() 
    : looping_(false), 
    quit_(false),
    calling_pending_functors_(false),
    threadId_(this_thread::get_id()),
    epoller_(make_unique<Epoller>(this)), 
    timer_manager_(make_unique<TimerManager>(this)),
    wakeup_fd_(createEventfd()),
    wakeup_channel_(this, wakeup_fd_)
{
    assert(!loopInThisThread && "Each thread can only have one EventLoop instance!");
    loopInThisThread = this;

    wakeup_channel_.setReadCallBack(bind(&EventLoop::handleWakeUp, this));
    wakeup_channel_.setEvents(EPOLLIN | EPOLLPRI); 
    wakeup_channel_.addToEpoller(); 
}

EventLoop::~EventLoop() {
    assert(!looping_); 
    loopInThisThread = nullptr; // 保证当前线程可再持有EventLoop实例.
}


bool EventLoop::isInLoopThread() {
    return threadId_ == std::this_thread::get_id(); 
}

void EventLoop::assertInLoopThread() {
    assert(isInLoopThread() && "EventLoop can only be used in the thread that created it!");
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

    while (!quit_) {
        active_channels_.clear(); 
        epoller_->poll(kPollTimeMs, &active_channels_); 
        for (auto& channel : active_channels_) {
            channel->handleEvents(); 
        }
        doPendingFunctors();   
    }
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
        perror("EventLoop:wakeup write");
    }
}

void EventLoop::handleWakeUp() {
    uint64_t one = 1;
    ssize_t n = read(wakeup_fd_, &one, sizeof one); 
    if (n != sizeof one) {
        perror("EventLoop:handleWakeUp error"); 
    }
}

void EventLoop::addToEpoller(Channel* channel) {
    assertInLoopThread(); 
    epoller_->epollAdd(channel); 
}

void EventLoop::modInEpoller(Channel* channel) {
    assertInLoopThread(); 
    epoller_->epollMod(channel); 
}

void EventLoop::delInEpoller(Channel* channel) {
    assertInLoopThread(); 
    epoller_->epollDel(channel); 
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
        perror("eventfd");
        abort(); 
    }
    return efd;
}

