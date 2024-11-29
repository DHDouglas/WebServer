#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL

#include <stdio.h>
#include <string>
#include <semaphore.h>
#include <list>
#include <mysql/mysql.h>


#include "../lock/locker.h"
#include "../log/log.h"


class ConnectionPool {
public:
    MYSQL* GetConnection();                // 获取数据库连接
    bool ReleaseConnection(MYSQL* conn);   // 释放连接
    int GetFreeConn();                     // 获取连接
    void DestroyPool();                    // 销毁所有连接

    // 单例模式(也可返回引用)
    static ConnectionPool* GetInstance();   

    void init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int CloseLog);

public:
    std::string m_url;            // 主机地址. 
    std::string m_Port;
    std::string m_User;           // 数据库用户名
    std::string m_PassWord;       // 数据库密码
    std::string m_DatabaseName;   // 使用数据库名
    int m_close_log;              // 日志开关


private:
    ConnectionPool();
    ~ConnectionPool();

    int m_MaxConn;    // 最大连接数
    int m_CurConn;    // 当前已使用的连接数
    int m_FreeConn;   // 当前空闲的连接数
    locker lock; 
    std::list<MYSQL *> connList;   // 连接池
    sem reserve;  
};


// 对Connection的RAII封装. 
class ConnectionRAII {
public:
    ConnectionRAII(MYSQL **con, ConnectionPool *connPool);
    ~ConnectionRAII();

private:
    MYSQL* conRAII;
    ConnectionPool *poolRAII; 
};
#endif