#ifndef LOCKER_H
#define LOCKER_H

#include <locker.h>
#include <pthread.h>
#include <exception>

// 定义线程同步封装类，互斥锁，条件变量，信号量
// 类内定义实现，隐式内联

//互斥锁封装类locker
class locker{
private:
    pthread_mutex_t m_mutex;
public:
    locker(){
        if(pthread_mutex_init(&m_mutex, NULL) != 0){
            throw std::exception();
        };
    };
    ~locker(){
        if(pthread_mutex_destroy(&m_mutex) != NULL){
            throw std::exception();
        }
    };
    bool lock(){
        //pthread_mutex_lock阻塞函数，一直等待加锁
        return pthread_mutex_lock(&m_mutex) == 0;
    };
    bool unlock(){
        return pthread_mutex_unlock(&m_mutex) == 0;
    };
};











#endif