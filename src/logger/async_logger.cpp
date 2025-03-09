#include <cassert>
#include <memory>
#include <cassert>

#include "async_logger.h"
#include "log_file.h"
#include "timestamp.h"



using namespace std;

AsyncLogger::AsyncLogger(const string& log_fname, 
                         const string& dir, 
                         off_t roll_size, 
                         int flush_interval)
    : flush_interval_(flush_interval),
    running_(false), 
    basename_(log_fname), 
    dir_(dir),
    roll_size_(roll_size),
    latch_(1), 
    current_buffer_(make_unique<Buffer>()),
    next_buffer_(make_unique<Buffer>()),
    buffers_()
{  
    assert(!log_fname.empty()); 
    current_buffer_->bzero();
    next_buffer_->bzero(); 
    buffers_.reserve(16);  
}

AsyncLogger::~AsyncLogger() {
    if (running_) {
        stop();
    }
}

void AsyncLogger::start() {
    if (running_) return; 
    assert(!thread_);  
    running_ = true;
    thread_ = make_unique<std::thread>(&AsyncLogger::threadFunc, this);  
    latch_.wait();   // 阻塞直至工作线程启动后才返回. 
}

void AsyncLogger::stop() {
    running_ = false;
    cond_.notify_all(); 
    if (thread_ && thread_->joinable()) {
        thread_->join(); 
    }
    thread_.reset(); 
}

void AsyncLogger::append(const char* logline, int len) {
    lock_guard<mutex> lock(mtx_);

    if (current_buffer_->avail() > len) {
        current_buffer_->append(logline, len); 
    } else {  // 当前buffer已满
        buffers_.push_back(std::move(current_buffer_));  // unique_ptr, 是移动语义.
        if (next_buffer_) {
            current_buffer_ = std::move(next_buffer_); 
        } else {
            current_buffer_ = make_unique<Buffer>();  // next_buffer_补充不及时的情况. 
        }
        current_buffer_->append(logline, len);
        cond_.notify_one(); 
    }
}

void AsyncLogger::threadFunc() {
    assert(running_ == true); 
    latch_.countDown(); 

    LogFile output(basename_, dir_, roll_size_, false);  // Logger后端只单线程写一个文件, 无需线程安全.
    BufferPtr new_buffer_1 = make_unique<Buffer>(); 
    BufferPtr new_buffer_2 = make_unique<Buffer>();
    new_buffer_1->bzero();
    new_buffer_2->bzero(); 
    BufferVector buffers_to_write; 
    buffers_to_write.reserve(16); 

    // 此循环是根据flushInterval_定时将buffers_内容写入文件. 
    while (running_) {
        assert(new_buffer_1 && new_buffer_1->length() == 0);
        assert(new_buffer_2 && new_buffer_2->length() == 0); 
        assert(buffers_to_write.empty()); 
        // 临界区内只进行swap和move.
        {
            unique_lock<mutex> lock(mtx_); 
            // buffers_为空时, 还没有需要写入文件的buffer块.
            cond_.wait_for(lock, 
                           Timestamp::secondsToDuration(flush_interval_), 
                           [this]{ return !buffers_.empty(); });
            buffers_.push_back(std::move(current_buffer_)); 
            current_buffer_ = std::move(new_buffer_1);
            buffers_to_write.swap(buffers_);  // 将空buffers组与buffers_交换.
            if (!next_buffer_) {              // 前端备用buffer块为空, 则补充
                next_buffer_ = std::move(new_buffer_2); 
            }
        }

        assert(!buffers_to_write.empty()); 

        if (buffers_to_write.size() > kMaxBuffersLimits) {
            // 若日志产生过快, 后端来不及写入文件, 积压buffers数量超过上限, 则丢弃多余日志, 防止占用过多内存.
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                Timestamp::now().toFormattedString().c_str(),
                buffers_to_write.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf))); // 丢弃日志的警告信息也写入文件.
            // 只保留前n个Buffer块, 其余直接丢弃. 
            buffers_to_write.erase(
                buffers_to_write.begin() + kBuffersReservedWhenOverStocked, 
                buffers_to_write.end()); 
        }

        // 写入文件
        for (const auto& buffer : buffers_to_write) {
            output.append(buffer->data(), buffer->length()); 
        }

        // buffers_to_write里保留2个Buffer块直接移动给new_buffer_1/2, 省略内存分配开销
        if (buffers_to_write.size() > 2) {
            buffers_to_write.resize(2); 
        }
        if (!new_buffer_1) { 
            assert(!buffers_to_write.empty());
            new_buffer_1 = std::move(buffers_to_write.back());
            buffers_to_write.pop_back();
            new_buffer_1->reset();  // 是Buffer.reset(), 重置Buffer中cur_指针位置.
        }
        if (!new_buffer_2) {
            assert(!buffers_to_write.empty());
            new_buffer_2 = std::move(buffers_to_write.back());
            buffers_to_write.pop_back();
            new_buffer_2->reset();  
        }

        buffers_to_write.clear(); 
        output.flush(); 
    }
    output.flush();
}