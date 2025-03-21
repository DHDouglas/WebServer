#include <atomic>
#include <condition_variable>
#include <vector>
#include <memory>
#include <mutex>

#include "thread.h"
#include "log_stream.h"
#include "count_down_latch.h"

class AsyncLogger {

public:
    // 日志文件名, 单份日志上限(满后切换新文件), 从buffer定期刷入日志文件的间隔秒数. 
    AsyncLogger(const std::string& basename, const std::string& dir, off_t roll_size, int flush_interval = 2); 
    ~AsyncLogger();
    void append(const char* logline, int len); 
    void start();
    void stop(); 

private:
    void threadFunc();  // 工作线程
    using Buffer = FixedBuffer<kLargeBuffer>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;  // unique? 
    using BufferPtr = BufferVector::value_type;

    const int flush_interval_; 
    std::atomic<bool> running_;
    std::string basename_; 
    std::string dir_; 
    const off_t roll_size_;   
    Thread thread_;
    CountDownLatch latch_;   // 用于阻塞start()直至工作线程启动
    std::mutex mtx_; 
    std::condition_variable cond_;
    BufferPtr current_buffer_;
    BufferPtr next_buffer_; 
    BufferVector buffers_;   // 待写入文件的已填满的缓冲. 

    static constexpr int kMaxBuffersLimits = 25;  // 允许积压的待写入文件的最大Buffer块数量
    static constexpr int kBuffersReservedWhenOverStocked = 2;  // 积压超过上限后, 最终保留的Buffer块数量(其余直接丢弃)
};