#include "log_file.h"

using namespace std;

AppendFile::AppendFile(string filename)
    : fp_(::fopen(filename.c_str(), "ae")),   // 'e' for O_CLOEXEC
    written_bytes_(0)
{
    assert(fp_); 
    ::setbuffer(fp_, buffer_, sizeof buffer_);  // 自己提供文件缓冲区
}

AppendFile::~AppendFile() {
    ::fclose(fp_); 
}

void AppendFile::append(const char* logline, size_t len) {
    size_t written = 0;
    while (written != len) {   // 保证写完
        size_t remain = len - written;
        size_t n = this->write(logline + written, remain); 
        if (n != remain) {
            int err = ferror(fp_); 
            if (err) {
                fprintf(stderr, "AppendFile::append() failed\n");
                break; 
            }
        }
        written += n;
    }
    written_bytes_ += written; 
}

void AppendFile::flush() {
    ::fflush(fp_); 
}

size_t AppendFile::write(const char* logline, size_t len) {
    // 无锁版本, 不对流加锁, 比fwrite更快, 适用于单线程环境. 
    // @param2 size_t size: 每个数据块大小(字节)
    // @param3 size_t n: 需写入的块数量
    return ::fwrite_unlocked(logline, 1, len, fp_); 
}

off_t AppendFile::writtenBytes() const {
    return written_bytes_; 
}


LogFile::LogFile(const std::string& basename, 
                 off_t roll_size, 
                 bool thread_safe, 
                 int fluash_interval, 
                 int check_every_n)
    : basename_(basename),
    roll_size_(roll_size),
    flush_interval_(fluash_interval),
    check_every_n_(check_every_n),
    count_(0),
    mtx_(thread_safe ? make_unique<mutex>() : nullptr),
    start_of_period_(0), 
    last_roll_(0),
    last_flush_(0)
{
    assert(basename.find('/') == string::npos);  
    rollFile();  
}

LogFile::~LogFile() = default; 


void LogFile::append(const char* logline, int len) {
    if (mtx_) {
        lock_guard<mutex> lock(*mtx_); 
        appendUnlocked(logline, len);
    } else {
        appendUnlocked(logline, len);
    }
}

void LogFile::flush() {
    if(mtx_) {
        lock_guard<mutex> lock(*mtx_);
        file_->flush();
    } else {
        file_->flush();
    }
}

void LogFile::appendUnlocked(const char* logline, int len) {
    file_->append(logline, len);
    if (file_->writtenBytes() > roll_size_) { // 当前文件写入量已超过roll_size_. 
        rollFile();
    } else {
        ++count_;
        if (count_ >= check_every_n_) {
            count_ = 0;
            time_t now = time(NULL); 
            time_t this_period = now / kRollIntervalSeconds_ * kRollIntervalSeconds_; 
            if (this_period > start_of_period_) {
                rollFile(); 
            } else if (now - last_flush_ > flush_interval_) {
                last_flush_ = now; 
                file_->flush(); 
            }
        }   
    }

}


bool LogFile::rollFile() {
    time_t now = 0;
    string fname = getLogFileName(basename_, &now);  // 取日志文件base名+当前时间
    time_t start = now / kRollIntervalSeconds_ * kRollIntervalSeconds_;  // 取整比较
    if (now > last_roll_) {
        last_roll_ = now;
        last_flush_ = now;
        start_of_period_ = start; 
        // 换新一份文件
        file_ = make_unique<AppendFile>(fname);  
        return true;
    }
    return false;
}

// 为base文件名加上后缀信息, 同时赋予now当前时间.
string LogFile::getLogFileName(const std::string& basename, time_t* now) {
    string fname;
    fname.reserve(basename.size() + 64); 
    fname = basename; 

    struct tm tm;
    *now = time(NULL); 
    // 带_r后缀: 线程安全版本, 使用用户提供的struct tm变量, 而非返回一个全局静态变量.
    localtime_r(now, &tm);   // !改用localtime.
    char timebuf[32]; 
    strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S", &tm); 
    fname += timebuf; 

    char pidbuf[32];
    snprintf(pidbuf, sizeof(pidbuf), ".%d", ::getpid()); 
    fname += pidbuf;

    fname += ".log";
    return fname;
}
