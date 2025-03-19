#include <timing_wheel.h>

#include <string>
#include <unistd.h>
#include <vector>

#include "any.h"
#include "eventloop.h"
#include "eventloop_threadpool.h"
#include "logger.h"

using namespace std;

void callback(Any& data) {
    auto msg = any_cast<string>(&data); 
    LOG_INFO << "callback: " << *msg;
}

string makeMsg(int i) {
    return to_string(i) + " start_time: " + Timestamp::now().toFormattedString();
}


// 测试定时事件触发
void testOnTimer() {
    EventLoop main_loop;
    EventLoopThread thread; 
    EventLoop* loop = thread.startLoop(); 
    int idle_seconds = 10; 
    TimingWheel timing_wheel(loop, idle_seconds, callback); 
    for (int i = 1; i <= 25; ++i) {
        timing_wheel.insert(makeMsg(i));  
        if (i % 2 == 0) {
            sleep(2); 
        }
    }
    main_loop.loop();
}

// 测试移除定时器
void testRemoveTimer() {
    EventLoop main_loop;
    EventLoopThread thread; 
    EventLoop* loop = thread.startLoop(); 
    int idle_seconds = 10; 
    TimingWheel timing_wheel(loop, idle_seconds, callback); 

    vector<TimingWheel::EntryWeakPtr> wk_ptrs;
    // 插入6个
    for (int i = 0; i < 6; ++i) {
        wk_ptrs.push_back(timing_wheel.insert(makeMsg(i)));  
    }
    sleep(3); 
    // 删除中间两个
    for (int i = 2; i < 4; ++i) {
        timing_wheel.remove(wk_ptrs[i]);  
    }
    sleep(3); 
    // 更新前2个
    for (int i = 0; i < 2; ++i) {
        timing_wheel.update(wk_ptrs[i]);  
    }
    main_loop.loop();
}

// 测试为不同EventLoop(不同线程)绑定TimerWheel
void testBindTimerWheelWithEventLoop() {
    EventLoop main_loop;
    EventLoopThreadPool loops_pool(&main_loop, ""); 
    int loop_cnt = 3; 
    loops_pool.setThreadNum(loop_cnt);
    loops_pool.start(); 

    int idle_seconds = 15; 
    auto loops = loops_pool.getAllLoops();
    for (auto& loop : loops) {
        loop->setContext(make_unique<TimingWheel>(loop, idle_seconds, callback));  
    }

    vector<TimingWheel::EntryWeakPtr> entrys_update;
    vector<TimingWheel::EntryWeakPtr> entrys_move;
    // 每个EventLoop所在线程各插入3个定时器, 再分别移除其中一个, 更新其中一个.
    for (auto& loop : loops) {
        for (int i = 0; i < 3; ++i) {
            auto wheel = any_cast<unique_ptr<TimingWheel>>(loop->getMutableContext()); 
            auto wkptr = (*wheel)->insert(makeMsg(i)); 
            if (i == 1) {
                entrys_move.push_back(wkptr); 
            } else if (i == 2) {
                entrys_update.push_back(wkptr); 
            }
        }
        sleep(4); 
    }
    // 各移除1个
    for (auto entry : entrys_move) {
        if (auto sptr = entry.lock()) {
            sptr->owner_wheel_->remove(entry); 
        }
    }
    sleep(1); 
    // 各更新1个
    for (auto entry : entrys_update) {
        if (auto sptr = entry.lock()) {
            sptr->owner_wheel_->update(entry); 
        }
    }
    main_loop.loop(); 
}


int main() {
    Logger::setLogLevel(Logger::INFO); 
    // testOnTimer();
    // testRemoveTimer();
    testBindTimerWheelWithEventLoop();
}



