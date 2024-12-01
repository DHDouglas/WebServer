#include "webserver.h"



int main(int argc, char** argv) {

    // 命令行解析
    // Config config;
    // config.parse_arg(argc, argv); 

    // // 初始化
    // server.init(config.PORT, user, passwd, databasename, config.LOGWrite,
    //     config.OPT_LINGER, config.TRIGMode, config.sql_num, config.thread_num,
    //     config.close_log, config.actor_model);

    // // 初始化日志类
    // server.log_write();

    // // 数据库
    // server.sql_pool();

    // // 线程池
    // server.thread_pool(); 

    // // 触发模式
    // server.trig_mode(); 

    // // 监听
    // server.eventListen();

    // // 运行
    // server.eventLoop(); 
    WebServer server(8080, false, 6); 

    // 初始化, 开启监听, epoll事件. 
    server.eventListen(); 

    // 事件循环: I/O多路复用. 
    server.eventLoop();
    return 0;
} 