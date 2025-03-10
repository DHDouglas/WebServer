#pragma once

#include <string>
#include <logger.h>

class Config {
public:
    Config() = default;
    explicit Config(int argc, char* argv[]) { parseArgs(argc, argv); }
    void printArgs() const; 

private:
    void parseArgs(int argc, char* argv[]); 
    void printHelp(const char* program) const; 

    void setLogLevel(const char* loglevel);
    std::string ensureAbsoluteRootPath(std::string path);
    
public:
    // ip地址
    std::string ip = "";
    // 端口号
    int port = 8080; 
    // HttpServer的IO线程数量(即subReactor数量)
    int num_thread = 5;
    // Web资源文件根目录
    std::string root_path_ = "./resources";
    // HttpConnection的超时时间
    double timeout_seconds_ = 30;
    // 日志文件名
    std::string log_file_name_ = "HttpServerLog";
    // 日志生成目录
    std::string log_dir_ = "./log"; 
    // 日志级别
    Logger::LogLevel log_level_ = Logger::INFO;
    // 单份日志文件上限(满后切换新文件)
    off_t log_rollsize_ = 500 * 1000 * 1000;    // 0.5GB;
    // 从buffer定期刷入日志文件的间隔秒数. 
    int log_flush_interval_seconds_ = 2;  
};