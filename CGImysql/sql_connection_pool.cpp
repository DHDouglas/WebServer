#include "sql_connection_pool.h"

using namespace std;

ConnectionPool::ConnectionPool() {
    m_CurConn = 0;
    m_FreeConn = 0;
}


ConnectionPool* ConnectionPool::GetInstance() {
    static ConnectionPool connPool;
    return &connPool; 
}


// 构造初始化
void ConnectionPool::init(string url, string User, string PassWord, string DBName, int Port, int MaxConn, int close_log) {
    m_url = url;
    m_Port = Port;
    m_User = User;
    m_PassWord = PassWord;
    m_DatabaseName = DBName; 
    m_close_log = close_log; 

    // 创建指定数量的连接
    for (int i = 0; i < MaxConn; ++i) {
        MYSQL* con = NULL; 
        con = mysql_init(con); 

        if (con == NULL) {
            LOG_ERROR("MySQL ERROR"); 
            exit(1); 
        }
        con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0); 

        if (con == NULL) {
            LOG_ERROR("MySQL Error");
            exit(1); 
        } 

        connList.push_back(con);
        ++m_FreeConn;  
    }

    reserve = sem(m_FreeConn);  // 初始化一个信号量; 
    m_MaxConn = m_FreeConn;     // 实际创建的连接数
}

// 从连接池中返回一个可用连接, 更新使用和空闲连接数
MYSQL *ConnectionPool::GetConnection() {
    MYSQL* con = NULL;   
    // ? 这里很明显存在条件竞争吧? 没加锁就判断connList, 在这步判断非空后, 其他线程可能竞争得到锁并将链表清空. 
    if (0 == connList.size()) return NULL; 

    reserve.wait();   // 信号量等待唤醒. 
    lock.lock();  

    con = connList.front();
    connList.pop_front(); 

    --m_FreeConn;
    ++m_CurConn; 

    lock.unlock();
    return con; 
}


// 释放当前使用的连接
bool ConnectionPool::ReleaseConnection(MYSQL* con) {
    if (NULL == con) return false;

    lock.lock(); 
    connList.push_back(con);
    ++m_FreeConn;
    --m_CurConn;
    lock.unlock();

    reserve.post(); 
    return true; 
}

// 销毁数据库连接池
void ConnectionPool::DestroyPool() {
    lock.lock(); 
    if (connList.size() > 0) {
        list<MYSQL*>::iterator it;
        for (it = connList.begin(); it != connList.end(); ++it) {
            MYSQL* con = *it; 
            mysql_close(con);   // 逐一关闭链接. 
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }
}

// 当前空闲的连接数
int ConnectionPool::GetFreeConn() {
    return this->m_FreeConn; 
}

ConnectionPool::~ConnectionPool() {
    DestroyPool();  
}

ConnectionRAII::ConnectionRAII(MYSQL **SQL, ConnectionPool* connPool) {
    // 从连接池中取出一个SQL连接, 构造RAII实例, 封装该链接. 
    *SQL = connPool->GetConnection();
    conRAII = *SQL; 
    poolRAII = connPool;  
}

ConnectionRAII::~ConnectionRAII() {
    poolRAII->ReleaseConnection(conRAII); 
}