#include <mutex>
#include <memory>

#include "logger.h"
#include "async_logger.h"

using namespace std;

constexpr off_t kRollSize = 400 * 1000;  // 0.4MB;
constexpr int kFlushInterval = 2;   // 2s

unique_ptr<AsyncLogger> logger; 
std::once_flag init_flag; 

void initAsyncLogger() {
    logger = make_unique<AsyncLogger>("TestLog", "log", kRollSize, kFlushInterval); 
    logger->start();  // 开启后端线程, 负责将AsycLogger缓冲区中的日志写入文件.
}

void outputToAsyncLoggerBuffer(const char* msg, int len) {
    std::call_once(init_flag, initAsyncLogger); 
    logger->append(msg, len);  // 从LogStream的缓冲区写入AsyncLogger缓冲区. 
}

// Logger写入AsyncLogging缓冲区不需要flush. 后者在写入日志文件时会自行调用LogFile的flush.
void flush() {}


int main() {
    // 为前端Logger注册日志输出回调
    Logger::setOutput(outputToAsyncLoggerBuffer);
    // Logger::setFlush(flush); 

    string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    for (int i = 0; i < 10000; ++i)
    {
      LOG_INFO << line << i;
      usleep(1000);
    }
}