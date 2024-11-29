#include <cassert>
#include <cstring>
#include "webserver.h"
#include "CGImysql/sql_connection_pool.h"


WebServer::WebServer() {
    // 建立MAX_FD个连接, 每个连接对应一个定时器. 

    // http_conn类对象
    users = new http_conn[MAX_FD];  // 初始时就创建这么多http连接? 

    // root文件夹路径
    char server_path[20];
    getcwd(server_path, 200);
    char root[6] = "/root";

    m_root = (char*) malloc(strlen(server_path) + strlen(root) + 1);  
    strcpy(m_root, server_path);
    strcat(m_root, root);

    // 定时器
    users_timer = new client_data[MAX_FD];  
}

WebServer::~WebServer() {
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete m_pool;
}


void WebServer::init(int port, string user, string passWord, string databaseName, int log_write, 
                     int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model)
{
    m_port = port;
    m_user = user;
    m_passWord = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log = close_log;
    m_actormodel = actor_model;
}


void WebServer::trig_mode() {
    
    switch (m_TRIGMode) {
        case 0 : {   // LT + LT
            m_LISTENTrigmode = 0;
            m_CONNTrigmode = 0;
        } break;

        case 1 : {  // LT + ET
            m_LISTENTrigmode = 0;
            m_CONNTrigmode = 1; 
        } break;

        case 2 : {  // ET + LT
            m_LISTENTrigmode = 1;
            m_CONNTrigmode = 0;
        } break;

        case 3 : {  // ET + ET
            m_LISTENTrigmode = 1;
            m_CONNTrigmode = 1; 
        } break;
    }
}


// 初始化日志
void WebServer::log_write() {
    if (0 == m_close_log) {
        if (1 == m_log_write) {
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 800); 
        } else {
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 0); 
        }
    }
}

// 初始化数据库连接池
void WebServer::sql_pool() {
    // 初始化数据库连接池
    m_connPool = ConnectionPool::GetInstance();
    m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num, m_close_log); 
    // 初始化数据库读取表
    // ? 为什么是调用http_conn对象的方法? 
    users->initmysql_result(m_connPool); 
}

// 线程池
void WebServer::thread_pool() {
    m_pool = new ThreadPool<http_conn>(m_actormodel, m_connPool, m_thread_num); 
}


// 建立监听
void WebServer::eventListen() {

    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0); 

    // 关闭连接
    if (0 == m_OPT_LINGER) {
        struct linger tmp = {0, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    } else if (1 == m_OPT_LINGER) {
        struct linger tmp = {1, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));  
    }

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address)); 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    address.sin_port = htons(m_port); 

    // 绑定端口
    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(m_listenfd, (struct sockaddr*)&address, sizeof(address)); 
    assert(ret >= 0);  

    ret = listen(m_listenfd, 5); 
    assert(ret >= 0);
    utils.init(TIMESLOT);   // 定时器? 


    // epoll创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER]; 
    m_epollfd = epoll_create(5); 
    assert(m_epollfd != -1); 

    //? 作用是什么? 
    utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode); 
    http_conn::m_epollfd = m_epollfd;

    // ? 作用是什么? 
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd); 
    assert(ret != -1); 
    utils.setnonblocking(m_pipefd[1]); 
    utils.addfd(m_epollfd, m_pipefd[0], false, 0); 

    // ? 为定时器添加信号. 
    utils.addsig(SIGPIPE, SIG_IGN); 
    utils.addsig(SIGALRM, utils.sig_handler, false); 
    utils.addsig(SIGTERM, utils.sig_handler, false); 

    alarm(TIMESLOT); 

    // 工具类, 信号和描述符基础操作
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd; 
}



/* 
 * 新连接建立请求
 * 服务器端关闭连接
 * 处理信号
 * 处理客户连接上接收到的数据
 * 
 */
void WebServer::eventLoop() {
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server) {
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1); 
        if (number < 0 && errno != EINTR) {
            LOG_ERROR("%s", "epoll failure"); 
            break;
        }   

        //

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd; 
            if (sockfd == m_listenfd) {

            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 服务器关闭连接, 移除对应的定时器
                util_timer *timer = user_timer[sockfd].timer;
                deal_timer(timer, sockfd); 
            } else if ((sockfd == m_pipefd[0])&& (events[i].events & EPOLLIN)) {
                // 处理信号
            } else 
        }
    }

}