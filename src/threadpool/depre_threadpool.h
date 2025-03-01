
#ifdef THREDDD

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <exception>
#include <list>
#include <cstdio>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

template <typename T>
class ThreadPool {
public:
    /* thread_number是线程池中线程的数量, max_requests是请求队列中最多允许的、等待处理的请求的数量 */
    ThreadPool(int actor_model, ConnectionPool *connPool, int thread_number = 8, int max_requests = 10000); 
    ~ThreadPool(); 
    // 两个版本, 一个有stat参数, 另一个没有. 
    bool append(T *request, int state);  
    bool append_p(T *request); 

private:
    // ? 为什么不直接把run放在worker里面, 而要单独抽离出一个run? 
    static void* worker(void *arg);  
    void run(); 

private:
    int m_thread_number;  
    int m_max_requests;  
    pthread_t *m_threads;          // 
    std::list<T*> m_workqueue;     // 请求队列
    locker m_queuelocker;          // 保护请求队列的互斥锁
    sem m_queuestat;               // 信号量用作条件变量, 通知是否有任务需要处理. 
    ConnectionPool *m_connPool;    // 数据库
    int m_actor_model;             // 模型切换 
};


template <typename T>
ThreadPool<T>::ThreadPool(int actor_model, ConnectionPool* connPool, int thread_number, int max_requests) : m_actor_model(actor_model), m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL), m_connPool(connPool) {
    if (thread_number <= 0 || max_requests <= 0) {
        throw std::exception(); 
    }

    m_threads = new pthread_t[m_thread_number]; 
    if (!m_threads) {
        throw std::exception(); 
    }

    // ? 创建线程后分离, 这样岂不是无法再join了? 如何获取执行结果呢? 
    for (int i = 0; i < thread_number; ++i) {
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();  
        }
        if (pthread_detach(m_threads[i])) {
            delete[] m_threads; 
            throw std::exception();  
        }
    }
}

template <typename T>
ThreadPool<T>::~ThreadPool() {
    delete[] m_threads;
}

template <typename T>
bool ThreadPool<T>::append(T *request, int state) {
    m_queuelocker.lock();  
    if (m_workqueue.size() >= m_max_requests) {   // 数量超过后禁止添加
        m_queuelocker.unlock(); 
        return false;
    }
    request->m_state = state;   // 这里request的类型不是还不知道吗, 怎么能假定其具有m_state成员呢? 
    m_workqueue.push_back(request);  
    m_queuelocker.unlock();  
    m_queuestat.post();     // 信号量. 
    return true; 
}

template <typename T>
bool ThreadPool<T>::append_p(T* request) {
    m_queuelocker.lock(); 
    if (m_workqueue.size() >= m_max_requests) {
        m_queuelocker.unlock();
        return false; 
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();  
    return true;
}

template <typename T>
void* ThreadPool<T>::worker(void *arg) {
    ThreadPool* pool = (ThreadPool* )arg;
    pool->run();  
    return pool; 
}


// 
template <typename T>
void ThreadPool<T>::run() {
    while (true) {
        m_queuestat.wait();  
        m_queuelocker.lock();
        if (m_workqueue.empty()) {
            m_workqueue.unlock(); 
            continue;
        }
        T* request = m_workqueue.front();  
        m_workqueue.pop_front();  
        m_queuelocker.unlock(); 
        if (!request) {  // 为什么workqueue里弹出的还可能为空? 这个设计是不是有问题. 
            continue;  
        }

        // 根据 "m_actor_model" 参数, 判断是走线程池, 还是连接池? 
        if (1 == m_actor_model) {
            // 根据request状态, 判断是否需要链接. 
            if (0 == request->m_state) {
                // 为什么是假定队列中的对象有`read_once()`? 
                if (request->read_once()) {
                    request->improv = 1;
                    ConnectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process(); 
                } else {
                    request->improv = 1;
                    request->timer_flag = 1; 
                }
            } else {
                if (request->write()) {
                    request->improv = 1; 
                } else {
                    request->improv = 1;
                    request->timer_flag = 1; 
                }
            }
        } else {
            ConnectionRAII mysqlcon(&request->mysql, m_connPool);
            request->proces(); 
        }
    }
}

#endif

#endif