/* 基于循环数组实现的 "有界阻塞队列", 元素存的是指向类型T的指针, m_back = (m_back + 1) % m_max_size; 
 */

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"
using namespace std;

template <class T>
class BlockQueue {
public:
    BlockQueue(int max_size = 1000); 
    ~BlockQueue();

    void clear();
    bool full(); 
    bool empty(); 
    bool front(T&);
    bool back(T&); 
    int size(); 
    int max_size(); 
    bool push(const T& item);
    bool pop(T& item); 
    bool pop(T& item, int ms_timeout);

private:
    locker m_mutex;
    cond m_cond; 

    T* m_array;        // 用一个数组作为循环队列. 
    int m_size;        // 
    int m_max_size;    // 
    int m_front;       // 队首和队尾元素下标.
    int m_back;        // 
};



template <typename T>
BlockQueue<T>::BlockQueue(int max_size) {
    if (max_size <= 0) {
        exit(-1); 
    }
    m_max_size = max_size; 
    m_array = new T[max_size];  // 创建指定数量的动态数组
    m_size = 0;
    m_front = -1;
    m_back = -1;  
}

template <typename T>
BlockQueue<T>::~BlockQueue() {
    m_mutex.lock();
    if (m_array != NULL) {
        delete[] m_array;
    }
    m_mutex.unlock(); 
}

template <typename T>
void BlockQueue<T>::clear() {
    m_mutex.lock(); 
    m_size = 0;
    m_front = -1;
    m_back = -1;
    m_mutex.unlock(); 
}

// 判断队列是否满了
template <typename T>
bool BlockQueue<T>::full() {
    m_mutex.lock();
    if (m_size >= m_max_size) {
        m_mutex.unlock();
        return true;
    }
    m_mutex.unlock();
    return false; 
}

// 判断队列是否为空
template <typename T>
bool BlockQueue<T>::empty() {
    m_mutex.lock();
    {
        m_mutex.unlock();
        return true;
    }
    m_mutex.unlock();
    return false; 
}

// 返回队首元素
template <typename T>
bool BlockQueue<T>::front(T& value) {
    m_mutex.lock();
    if (0 == m_size) {
        m_mutex.unlock();
        return false;
    }
    value = m_array[m_front];
    m_mutex.unlock();
    return true;
}

// 返回队尾元素
template <typename T>
bool BlockQueue<T>::back(T& value) {
    m_mutex.lock();
    if (0 == m_size) {
        m_mutex.unlock();
        return false;
    }
    value = m_array[m_back];
    m_mutex.unlock();
    return true;
}


template <typename T>
int BlockQueue<T>::size() {
    int tmp = 0;
    m_mutex.lock();
    tmp = m_size;
    m_mutex.unlock();
    return tmp;
}

template <typename T>
int BlockQueue<T>::max_size() {
    int tmp = 0;
    m_mutex.lock();
    tmp = m_max_size;
    m_mutex.unlock();
    return tmp;
}

// ? 下面使用boardcast唤醒全部线程, 是否必要? 
template <typename T>
bool BlockQueue<T>::push(const T& item) {
    m_mutex.lock(); 
    if (m_size >= m_max_size) {  // 唤醒阻塞着的pop. 
        m_cond.broadcast();  
        m_mutex.unlock(); 
        return false;  
    }
    m_back = (m_back + 1) % m_max_size;
    m_array[m_back] = item;
    m_size++;
    m_cond.broadcast();
    m_mutex.unlock(); 
    return true;
}


template <typename T>
bool BlockQueue<T>::pop(T& item) {
    m_mutex.lock(); 
    while (m_size <= 0) {
        if (!m_cond.wait(m_mutex.get())) {
            m_mutex.unlock(); 
            return false;   // 什么情况下, 才会return false? 
        }
    }
    m_front = (m_front + 1) % m_max_size;
    item = m_array[m_front];
    m_size--;
    m_mutex.unlock();  
    return true;
}


// 超时处理, 传入参数为毫秒. 
template <typename T>
bool BlockQueue<T>::pop(T& item, int ms_timeout) {
    struct timespec t = {0, 0}; 
    struct timeval now = {0, 0,};
    gettimeofday(&now, NULL);  
    if (m_size <= 0) {
        t.tv_sec = now.tv_sec + ms_timeout / 1000;
        t.tv_nsec = (ms_timeout % 1000) * 1000; 
        if (!m_cond.timewait(m_mutex.get(), t)) {
            m_mutex.unlock(); 
            return false; 
        }
    }

    if (m_size <= 0) {
        m_mutex.unlock();
        return false;
    }

    m_front = (m_front + 1) % m_max_size;
    item = m_array[m_front];
    m_size--;
    m_mutex.unlock();
    return true;
}

#endif
