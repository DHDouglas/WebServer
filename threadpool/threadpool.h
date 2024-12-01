/* 线程池实现 (基于无界队列) */

#include <condition_variable>
#include <mutex>
#include <functional>
#include <queue>
#include <stdexcept>
#include <vector>
#include <thread>
#include <future>

class ThreadPool {
public:
    ThreadPool(size_t numThreads); 
    ~ThreadPool(); 

    // 提交任务
    template<typename Func, typename... Args> 
    auto submit(Func&& f, Args&&... args)
    -> std::future<std::invoke_result_t<Func, Args...>>;  

    // 结束线程池
    void shutdown(); 
    
    // 禁止拷贝
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;  

private:
    void worker_thread();                 // 工作线程函数 

    std::mutex mtx;
    std::condition_variable cv; 
    bool stop;
    std::vector<std::thread> workers;   // 工作线程
    std::queue<std::function<void()>> tasks;             // 任务队列
};


template<typename Func, typename... Args>
auto ThreadPool::submit(Func&& f, Args&&... args) 
-> std::future<std::invoke_result_t<Func, Args...>> {
    // 封装任务, 加入队列. 
    using ReturnType = decltype(f(args...)); 

    auto ptr_task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
    );
    
    // 获取futurue. 
    auto res = ptr_task->get_future(); 
    {
        std::lock_guard<std::mutex> lock(mtx); 
        if (stop) {  // 线程池已结束, 不再允许入队任务
            throw std::runtime_error("enqueue on stopped ThreadPool"); 
        } 
        tasks.emplace([ptr_task]{ (*ptr_task)();  });  // 加入队列. 
    }

    cv.notify_one();  
    return res;   // 返回std::future<>; 
}