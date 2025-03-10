#include "eventloop_threadpool.h"

#include <cassert>

#include "eventloop.h"

using namespace std;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop, const string& name)
    : base_loop_(base_loop),
    name_(name),
    started_(false),
    num_threads_(0),
    next_(0)
{
}


EventLoopThreadPool::~EventLoopThreadPool() {
}


void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    assert(!started_);
    base_loop_->assertInLoopThread(); 
    started_ = true;
    for (int i = 0; i < num_threads_; ++i) {
        threads_.push_back(make_unique<EventLoopThread>(cb, name_ + to_string(i))); 
        loops_.push_back(threads_.back()->startLoop()); 
    }
    if (num_threads_ == 0 && cb) {  
        cb(base_loop_);     // 单线程, baseloop所在线程即作为io线程. 
    }
}

// 以round-robin方式选取
EventLoop* EventLoopThreadPool::getNextLoop() {
    base_loop_->assertInLoopThread(); 
    assert(started_); 
    EventLoop* loop = base_loop_;  
    if (!loops_.empty()) {
        loop = loops_[next_]; 
        ++next_; 
        if (static_cast<size_t>(next_) >= loops_.size()) {
            next_ = 0; 
        }
    } 
    return loop; 
}