#include "timer.h"
#include "eventloop.h"
#include "timestamp.h"
#include <iterator>

using namespace std;


TimerManager::TimerManager(EventLoop* loop) 
    : loop_(loop),
    timerfd_(create_timerfd()),
    timerfd_channel_(loop_, timerfd_), 
    timers_()
{   
    // 设置timerfd的回调
    timerfd_channel_.setReadCallback(bind(&TimerManager::handleExpiredTimer, this));
    timerfd_channel_.enableReading(); 
};

TimerManager::~TimerManager() {
    timerfd_channel_.disableAll(); 
    timerfd_channel_.removeFromEpoller(); 
    close(timerfd_);
}

int TimerManager::create_timerfd() {
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC); 
    if (tfd < 0) {
        LOG_SYSFATAL << "TimerManager::create_timerfd() timerfd_create";
    }
    return tfd; 
}

weak_ptr<Timer> TimerManager::addTimer(TimerCallback cb, Timestamp exp, Duration interval) {
    // TimeManager是无锁的, 非线程安全, 故必须将`addTimer`操作放在其所属EvenLoop所在线程中执行. 
    auto ptr_timer = make_shared<Timer>(std::move(cb), exp, interval); 
    loop_->runInLoop(bind(&TimerManager::addTimerInLoop, this, ptr_timer)); 
    return ptr_timer; 
}

void TimerManager::addTimerInLoop(const shared_ptr<Timer>& ptr_timer) {
    loop_->assertInLoopThread();  
    if (insert(ptr_timer)) {
        checkUpdateTimerfd(); 
    }
}

bool TimerManager::insert(const shared_ptr<Timer>& ptr_timer) {
    loop_->assertInLoopThread();  
    Timestamp exp = ptr_timer->getExpiration(); 
    bool update_timerfd = false; 
    auto it = timers_.begin();
    if (it == timers_.end() || exp < it->first) {
        update_timerfd = true; 
    }
    
    auto res = timers_.insert(Entry(exp, ptr_timer)); 
    assert(res.second); 
    return update_timerfd; 
}

void TimerManager::handleExpiredTimer() {
    loop_->assertInLoopThread(); 
    // 1.获取当前时间
    auto now = Timestamp::now(); 
    // 2.读取timerfd
    uint64_t clicks; 
    ssize_t n = read(timerfd_, &clicks, sizeof(clicks)); 
    if (n != sizeof(clicks)) {
        LOG_SYSFATAL << "TimerManager::handleExpiredTimer() read";
    }
    // 3.从set中获取&移除已到期的定时器列表, 记入expired.
    vector<Entry> expired; 
    Entry sentry = make_pair(Timestamp::now(), 
        shared_ptr<Timer>(reinterpret_cast<Timer*>(UINTPTR_MAX), [](Timer*){}));  // 哨兵值, 必须自定义删除器, 避免对UINTPTR_MAX地址释放内存.
    auto it = timers_.lower_bound(sentry);  // 获取指向第一个未到期的Timer的迭代器
    assert(it == timers_.end() || now < it->first);  
    copy(timers_.begin(), it, back_inserter(expired));  
    timers_.erase(timers_.begin(), it); 

    // 4.处理每个到期的定时器: 执行回调
    calling_expired_timers_ = true;
    canceling_timers_.clear(); 
    for (const auto& p : expired) {
        p.second->run();   // 执行回调. 
    }
    calling_expired_timers_ = false; 

    // 5.对于周期性事件, 更新到期时间, 重新加入计时. 
    for (const auto& p : expired) {
        auto& timer = p.second; 
        // 若timer在canceling_timers_中, 表明需移除, 故不再重新加入.
        if (timer->repeatable() && (canceling_timers_.find(timer) == canceling_timers_.end())) {
            timer->restart(now); 
            insert(timer); 
        }
    }
    // 全部插入后, 重新取最小到期时间, 作为timerfd到期时间.
    checkUpdateTimerfd();  
}


void TimerManager::checkUpdateTimerfd() {
    // 根据timers中的最小到期时间, 重设timerfd的到期时间. 
    // cout << "trigger" << endl; 
    if (timers_.empty()) return; 
    auto it = timers_.begin(); 
    struct timespec ts = it->second->getExpiration().getDurationFromNowAsTimeSpec(); 

    struct itimerspec new_value;
    memset(&new_value, 0, sizeof(new_value));
    new_value.it_value = ts;
    if (timerfd_settime(timerfd_, 0, &new_value, nullptr) < 0) {
        LOG_SYSFATAL << "TimerManager::checkUpdateTimerfd() timerfd_settime";
    }
}

void TimerManager::removeTimer(const weak_ptr<Timer>& wk_ptr) {
    loop_->runInLoop(bind(&TimerManager::removeTimerInLoop, this, wk_ptr)); 
}

void TimerManager::removeTimerInLoop(const weak_ptr<Timer>& wk_ptr) {
    // 注意: 处理自注销的情况——Timer的回调任务的注销该Timer本身, 
    // 即此时正位于执行handleExpiredTimer的过程, 该Timer已不在timers_中而在expired中.
    loop_->assertInLoopThread(); 
    if (shared_ptr<Timer> s_ptr = wk_ptr.lock()) {
        auto it = timers_.find(Entry(s_ptr->getExpiration(), s_ptr)); 
        if (it != timers_.end()) {
            timers_.erase(it);  
        } else if (calling_expired_timers_) {  // 自注销
            canceling_timers_.insert(s_ptr);  
        }
    }
}