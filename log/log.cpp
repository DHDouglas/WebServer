#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/time.h>   // 更精细的时间? 
#include <pthread.h>

#include "log.h"

using namespace std;

Log::Log() {
    m_count = 0;
    m_is_async = false;
}

Log::~Log() {
    if (m_fp != NULL) {
        fclose(m_fp); 
    }
}

// 异步需要设置阻塞队列的长度, 同步不需要设置
bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size) {
    // 如果设置了max_queue_size, 则设置为异步写. 
    if (max_queue_size >= 1) {
        m_is_async = true;
        m_log_queue = new BlockQueue<string>(max_queue_size);
        pthread_t tid; 
        // flush_log_thread为回调函数, 这里表示创建线程异步写日志. 
        pthread_create(&tid, NULL, flush_log_thread, NULL);  
    }

    m_close_log = close_log;
    m_log_buf_size = log_buf_size; 
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size); 
    m_split_lines = split_lines;  

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;  

    const char*p = strrchr(file_name, '/');  
    char log_full_name[256] = {0};   // 日志文件名(base name). 
    // 在日志文件名中加上时间. 
    if (p == NULL) {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    } else {
        strcpy(log_name, p + 1);   // basename
        strncpy(dir_name, file_name, p - file_name + 1); 
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday; 
    
    m_fp = fopen(log_full_name, "a");
    if (m_fp == NULL) {
        return false;
    }
    return true;
}


// 写日志
void Log::write_log(int level, const char *format, ...) {
    struct timeval now = {0, 0};  
    gettimeofday(&now, NULL); 
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;  

    // 为什么这里不用string, 直接赋值==呢?
    char s[16] = {0};  
    switch(level) {
        case 0: 
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "warn:"); 
            break;
        case 3:
            strcpy(s, "[erro]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }

    // 写入一个log, 增加日志行数. 
    m_mutex.lock(); 
    m_count++; 

    // 日志文件每天一个; 当日志文件中行数已达到最大值m_split_lines时, 打开新日志文件;
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) {
        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};

        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (m_today != my_tm.tm_mday) {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name); 
            m_today= my_tm.tm_mday;
            m_count = 0;
        } else {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines); 
        }
        m_fp = fopen(new_log, "a"); 
    }
    m_mutex.unlock();  

    va_list valst;
    va_start(valst, format); 

    // 写入日志内容
    string log_str; 
    m_mutex.lock(); 
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;  // ? 为什么需要再将缓冲区内容赋给string? 为了压入队列吗? 
    m_mutex.unlock(); 

    // 若是异步写入模式, 且队列非空, 则将日志内容压入队列. 
    if (m_is_async && !m_log_queue->full()) {
        m_log_queue->push(log_str);  // 以string为对象, 
    } else {
        // 同步模式, 则直接写入文件; 
        m_mutex.lock(); 
        fputs(log_str.c_str(), m_fp);  // ? 直接取m_buf不好吗. 
        m_mutex.unlock();  
    }

    va_end(valst);
}


void Log::flush(void) {
    m_mutex.lock(); 
    fflush(m_fp);
    m_mutex.unlock(); 
}

