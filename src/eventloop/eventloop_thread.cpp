#include "eventloop_thread.h"
#include "channel.h"
#include <mutex>

using namespace std;


EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const string& name)
    : loop_(nullptr),
    thread_(bind(&EventLoopThread::threadFunc, this), name), 
    exiting_(false),
    init_callback_(cb) {}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_) {  // not 100% race-free, eg. threadFunc could be running callback_.
         // still a tiny chance to call destructed object, if threadFunc exits just now.
        // but when EventLoopThread destructs, usually programming is exiting anyway.
        // => 有可能进入这里时, loop_已经为空了, 例如threadFunc()已执行完成.
        // FIXME: 加锁也没用. EventLoop生命周期不受EventLoopThread管理. 例如
        loop_->quit();
        thread_.join(); 
    }
}

EventLoop* EventLoopThread::startLoop() {
    assert(!thread_.isStarted()); 
    thread_.start(); 
    EventLoop* loop = nullptr;
    {
        unique_lock<mutex> lock(mtx_);
        cond_.wait(lock, [this]{return loop_ != nullptr; });
        loop = loop_; 
    }
    return loop;
}

void EventLoopThread::threadFunc() {
    EventLoop loop;  // 创建EventLoop. 其构造函数本身保证每个线程下只能存在一个EventLoop对象.
    if (init_callback_) {
        init_callback_(&loop); 
    }

    {
        unique_lock<mutex> lock(mtx_);
        loop_ = &loop; 
        cond_.notify_one(); 
    }
    loop.loop(); 

    lock_guard<mutex> lock(mtx_); 
    loop_ = nullptr; 
}