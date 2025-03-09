#include <getopt.h>
#include <linux/limits.h>    // for PATH_MAX
#include <iostream>
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "log_file.h"

using namespace std;

void Config::parseArgs(int argc, char* argv[]) {
    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},              // --help
        {"ip", required_argument, 0, 'i'},          // --ip <ip address>
        {"port", required_argument, 0, 'p'},        // --port <port>
        {"thread", required_argument, 0, 'j'},      // --thread <num_threads>
        {"path", required_argument, 0, 'r'},        // --path <web root path>
        {"timeout", required_argument, 0, 't'},     // --timeout <seconds>
        {"logfname", required_argument, 0, 'f'},    // --logfname <file_name>
        {"logdir", required_argument, 0, 'R'},      // --logdir <dir path>
        {"loglevel", required_argument, 0, 'l'},    // --loglevel <level>
        {"logrollsize", required_argument, 0, 's'}, // --logroolsize <size>
        {"logflush", required_argument, 0, 'u'},    // --logflush <interval_seconds>
        {0, 0, 0, 0}  // 结束标志
    };

    const char* optstring = "hi:p:j:r:t:f:R:l:s:u:";
    int opt;
    while ((opt = getopt_long(argc, argv, optstring,long_options, nullptr)) != -1) {
        switch (opt) {
            case 'h': {
                printHelp(argv[0]); 
                exit(0); 
            } break;
            case 'i': ip = optarg;   break;
            case 'p': port = stoi(optarg);  break;
            case 'j': num_thread = stoi(optarg); break;
            case 'r': root_path_ = optarg; break;
            case 't': timeout_seconds_ = stoi(optarg); break;
            case 'f': log_file_name_ = optarg; break;
            case 'R': log_dir_ = optarg; break;
            case 'l': setLogLevel(optarg); break;
            case 's': log_rollsize_ = stoi(optarg); break;
            case 'u': log_flush_interval_seconds_ = stoi(optarg); break;
            case '?': {
                cerr << "Unknown option encountered.\n";
                exit(1); 
            } 
        }
    }
    LogFile::ensureDirExists(log_dir_); 
    root_path_ = ensureAbsoluteRootPath(root_path_);
    log_dir_ = ensureAbsoluteRootPath(log_dir_);
}

void Config::printHelp(const char* program) const {
    cout << "Usage: " << program << " [options]\n"
         << "Options:\n"
         << "  -h, --help                Show this help message\n"
         << "  -i, --ip <ip_address>     Set server ip address\n"
         << "  -p, --port <num>          Set server port (default: 8080)\n"
         << "  -j, --thread <num>        Set the number of IO threads(subReactors)\n"
         << "  -r, --path <dir>          Set the root path of Web resources\n"
         << "  -t, --timeout <num>       Set the timeout seconds of http connection\n"
         << "  -f, --logfname <name>     Set the name of log file. Default logging to stdout\n"
         << "  -R, --logdir <dir>        Set the dir of log file.\n"
         << "  -l, --loglevel <num>      Set the log level. 0:TRACE, 1:DEBUG, 2:INFO, 3:WARN, 4:ERROR, 5:FATAL\n"
         << "  -s, --logrollsze <num>    Set the max size of one single log file in bytes\n"
         << "  -u, --logflush <num>      Set the interval seconds of flush loginfo from buffer to log file\n";
}

void Config::setLogLevel(const char* loglevel) {
    int idx = stoi(loglevel); 
    switch (idx) {
        case 0: log_level_ = Logger::LogLevel::TRACE; break;
        case 1: log_level_ = Logger::LogLevel::DEBUG; break;
        case 2: log_level_ = Logger::LogLevel::INFO; break;
        case 3: log_level_ = Logger::LogLevel::WARN; break;
        case 4: log_level_ = Logger::LogLevel::ERROR; break;
        case 5: log_level_ = Logger::LogLevel::FATAL; break;
    }
}

string Config::ensureAbsoluteRootPath(string path) {
    // 若为相对路径, 则加上cwd作为前缀. 
    if (!(!path.empty() && path[0] == '/')) {
        char cwd[PATH_MAX]; 
        if (getcwd(cwd, sizeof(cwd)) == nullptr) {
            cerr << "Config::setRootPath getcwd failed" << endl;
            exit(1); 
        }
        path = string(cwd) + "/" + path;
    }

    // 路径解析, 去掉".", ".." 得到规范化路径.
    char resolved_path[PATH_MAX];
    if (realpath(path.c_str(), resolved_path) == nullptr) {
        cerr << "Config::setRootPath realpath failed: " << path << endl;;
        exit(1); 
    }
    return resolved_path;
}


void Config::printArgs() const {
    cout << "Options:\n"
         << "  ip: " << (ip.empty() ? "0.0.0.0" : ip) << "\n"
         << "  port: " << port << "\n"
         << "  the number of IO threads: "  << num_thread << "\n"
         << "  the web root path: " << root_path_ << "\n"
         << "  the timeout seconds of http connection: " << timeout_seconds_ << "\n"
         << "  log file name: " << log_file_name_ <<"\n"
         << "  log dir: " << log_dir_ << "\n"
         << "  log level: " << LogLevelName[log_level_] << "\n"
         << "  log rollsze(max bytes in a single log file): " << log_rollsize_ << "\n"
         << "  log flush interval seconds (flush from buffer to log file): " << log_flush_interval_seconds_ << "\n";
}