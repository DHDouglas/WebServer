/* 时间轮
 * 其本身向TimerManager注册一个周期性定时器, 每秒触发一次TimingWheel::onTimer()
 * onTimer里处理当前槽内的Entry, 通过clear()清空, 析构触发回调.
 * bucket以shared_ptr<Entry>方式进行管理, 故bucket清空时, 触发Entry析构 => 触发用户回调
 */

#include "timing_wheel.h"

#include "timestamp.h"
#include "eventloop.h"
#include "logger.h"

using namespace std;

static TimingWheel::Callback callback_;       

TimingWheel::Entry::Entry(TimingWheel* wheel, Any data)
    :owner_wheel_(wheel),
    data_(std::move(data)),
    bucket_num_(-1)
{ }

TimingWheel::Entry::~Entry() {
    // if (owner_wheel_ && owner_wheel_->callback_) {
    //     owner_wheel_->callback_(data_); 
    // }
}

TimingWheel::TimingWheel(EventLoop* loop, int idle_seconds, Callback cb)
    : loop_(loop),
    idle_seconds_(idle_seconds),
    begin_idx_(0),
    end_idx_(idle_seconds - 1),
    buckets_(idle_seconds),  // 槽数即超时时长, 每秒对应一个槽.
    callback_(std::move(cb))
{   
    // 基准定时器: 每秒处理一次槽
    base_timer_ = loop_->runEvery(Timestamp::secondsToDuration(1), bind(&TimingWheel::onTimer, this));  
}

TimingWheel::EntryWeakPtr TimingWheel::insert(Any data) {
    EntryPtr ptr = make_shared<Entry>(this, std::move(data)); 
    loop_->runInLoop(bind(&TimingWheel::insertInLoop, this, ptr)); 
    return ptr;
}

void TimingWheel::update(const EntryWeakPtr& entry_wkptr) {
    loop_->runInLoop(bind(&TimingWheel::updateInLoop, this, entry_wkptr)); 
}

void TimingWheel::remove(const EntryWeakPtr& entry_wkptr)  {
    loop_->runInLoop(bind(&TimingWheel::removeInLoop, this, entry_wkptr)); 

}

void TimingWheel::onTimer() {
    LOG_TRACE << "TimingWheel::onTimer:  bucket[" << begin_idx_ << "]: "<< buckets_[begin_idx_].size();
    for (auto& entry : buckets_[begin_idx_]) {
        if (entry->owner_wheel_ && entry->owner_wheel_->callback_) {
            entry->owner_wheel_->callback_(entry->data_); 
        }
    }
    buckets_[begin_idx_].clear();  
    begin_idx_ = (begin_idx_ + 1) % idle_seconds_;
    end_idx_ = (end_idx_ + 1) % idle_seconds_;
}


void TimingWheel::insertInLoop(const EntryPtr& ptr) {
    loop_->assertInLoopThread(); 
    buckets_[end_idx_].push_back(ptr); 
    ptr->bucket_num_ = end_idx_;
    ptr->pos_ = --buckets_[end_idx_].end();
}

void TimingWheel::updateInLoop(const EntryWeakPtr& entry_wkptr) {
    loop_->assertInLoopThread(); 
    if (auto ptr = entry_wkptr.lock()) {
        int ori_idx = ptr->bucket_num_; 
        if (ori_idx != end_idx_) {
            buckets_[ori_idx].erase(ptr->pos_); 
            buckets_[end_idx_].push_back(ptr);
            ptr->bucket_num_ = end_idx_;
            ptr->pos_ = --buckets_[end_idx_].end();
        }
    }
}

void TimingWheel::removeInLoop(const EntryWeakPtr& entry_wkptr) {
    loop_->assertInLoopThread(); 
    if (auto ptr = entry_wkptr.lock()) {
        int ori_idx = ptr->bucket_num_; 
        buckets_[ori_idx].erase(ptr->pos_); 
    }
}