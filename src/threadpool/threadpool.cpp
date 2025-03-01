#include "threadpool.h"
#include <mutex>


ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this]{ worker_thread(); }); 
    }
}

ThreadPool::~ThreadPool() {
    shutdown(); 
}


void ThreadPool::worker_thread() {
    while (true) {
        std::function<void()> task; 
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]{ return stop || !tasks.empty(); });   // 等待任务或线程池关闭. 
            if (stop && tasks.empty()) {
                return;   // 停止且没有任务, 退出线程; 
            }
            task = std::move(tasks.front());  // 获取任务
            tasks.pop(); 
        }
        task();  // 执行任务
    }
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mtx); 
        stop = true; 
    }
    cv.notify_all();  

    for (auto& t: workers) {
        if (t.joinable()) {
            t.join();  
        }
    }
}