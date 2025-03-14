#include "thread.h"

#include <cassert>
#include <sys/prctl.h>   // for prctl(), 设置linux线程名. 
#include <unistd.h>


using namespace std;

namespace CurrentThread {

thread_local int t_cached_tid = 0; 
thread_local char t_tid_string[32]; 
thread_local int t_tid_string_len = 6; 
const char* t_thread_name = "unknown"; 
static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

void cachedTid() {
    if (t_cached_tid == 0) {
        t_cached_tid = ::gettid(); 
        t_tid_string_len = snprintf(
            t_tid_string, sizeof(t_tid_string), "%7d ", t_cached_tid); 
    }
}

bool isMainThread() {
    return getTid() == ::getpid();  // 主线程的gettid()即为getpid()进程id.
}
    
} // namespace CurrentThread


// 初始化主线程的线程id, 线程名.
void afterFork()
{
  CurrentThread::t_cached_tid = 0; 
  CurrentThread::t_thread_name = "main";
  CurrentThread::getTid(); 
  // no need to call pthread_atfork(NULL, NULL, &afterFork);
}

struct ThreadNameInitializer {
public:
  ThreadNameInitializer() {
    CurrentThread::t_thread_name = "main";
    CurrentThread::getTid();
    pthread_atfork(NULL, NULL, &afterFork);
  }
};

ThreadNameInitializer init;


atomic<int> Thread::num_created_(0); 

Thread::Thread(ThreadFunc func, const string& name)
    : started_(false),
    tid_(0), 
    func_(std::move(func)),
    name_(name),
    latch_(1)
{
    ++num_created_; 
    if (name_.empty()) {  // 设置默认线程名, 加上线程id.
        name_ = "Thread" + to_string(num_created_.load()); 
    }
}

Thread::~Thread() {
    if (thread_.joinable()) {
        thread_.detach();  // 分离, 避免未join的线程导致崩溃. 
    }
}


void Thread::threadFunc() {
    tid_ = CurrentThread::getTid(); 
    latch_.countDown();  
    CurrentThread::t_thread_name = name_.empty() ? "Thread" : name_.c_str(); 
    // Linux API, 设置线程名. 设置后, 可在htop, top, gdb工具中看到线程名.
    ::prctl(PR_SET_NAME, CurrentThread::t_thread_name);  
    try {
        func_();  // 线程工作函数
        CurrentThread::t_thread_name = "finished"; 
    } catch (const exception& ex) {
        CurrentThread::t_thread_name = "crashed"; 
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str()); 
        fprintf(stderr, "reason: %s\n", ex.what()); 
        abort(); 
    } catch (...) {
        CurrentThread::t_thread_name = "crashed"; 
        fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
        throw;  // rethrow
    }
}


void Thread::start() {
    assert(!started_ && "Thread already started!");
    started_ = true; 
    thread_ = thread([this]{ threadFunc(); }); 
    latch_.wait();  // 等待线程启动成功, 赋予tid后才返回.
    assert(tid_ > 0); 
}

void Thread::join() {
    assert(started_ && "Thread not started"); 
    assert(thread_.joinable() && "Thread is not started or already joined!");
    thread_.join();
}