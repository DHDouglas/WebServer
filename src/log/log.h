#ifndef LOG_H
#define LOG_H

#include <string>
#include <stdarg.h>
#include <mutex>
#include <pthread.h> 
#include "block_queue.h"

using namespace std;

class Log {
public:
    static Log* get_instance() {
        static Log instance;
        return &instance;    
    }

    static void* flush_log_thread(void *args) {
        Log::get_instance()->async_write_log();  
    }
    
    //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    void write_log(int level, const char* format, ...); 

    void flush(void); 

private:
    Log();
    virtual ~Log();  // ? 为什么要定义为虚函数, 且实际上有进行实现.

    // 异步日志记录
    void* async_write_log() {
        string single_log; 
        // 从阻塞队列中循环地取出一个日志string, 写入文件
        while(m_log_queue->pop(single_log)) {  // 未存在时, 阻塞; 
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp); 
            m_mutex.unlock(); 
        }
        return NULL;
    }

private:
    char dir_name[128]; //路径名
    char log_name[128]; //log文件名
    int m_split_lines;  //日志最大行数
    int m_log_buf_size; //日志缓冲区大小
    long long m_count;  //日志行数记录
    int m_today;        //日志文件按天分类, 故记录当前时间是那一天. 
    FILE *m_fp;         //打开log的文件指针
    char *m_buf;
    BlockQueue<string> *m_log_queue;   // 自定义的阻塞队列
    bool m_is_async;                    // flag: 标记是否同步写入. 
    locker m_mutex;
    int m_close_log; //关闭日志


};



// ? 为什么用宏的方式实现, 而不是成员函数? 
// 日志方法
#define LOG_DEBUG(format, ...) if (0 == m_close_log) {Log::get_instance()->write_log(0, format, ##__VA__ARGS__); LOG::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#endif