#include <memory>
#include <vector>

#include "channel.h"
#include "eventloop_thread.h"

class EventLoopThreadPool {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

public:
    EventLoopThreadPool(EventLoop* base_loop, const std::string& name); 
    ~EventLoopThreadPool(); 

    void setThreadNum(int num_thread) { num_threads_ = num_thread; }
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // valid after calling start()
    /// round-robin
    EventLoop* getNextLoop();

    bool isStarted() const { return started_; }
    const std::string& getName() const { return name_; }


private:
    EventLoop* base_loop_;
    std::string name_; 
    bool started_; 
    int num_threads_;
    int next_;  
    std::vector<std::unique_ptr<EventLoopThread>> threads_; 
    std::vector<EventLoop*> loops_;  // 为什么既持有EventLoopThread, 又还要持有其指针? 

};