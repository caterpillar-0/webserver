#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <exception>
#include <semaphore.h>

// 定义线程同步封装类，互斥锁，条件变量，信号量
// 类内定义实现，隐式内联

//互斥锁封装类locker
class locker{
private:
    pthread_mutex_t m_mutex;
public:
    locker(){
        if(pthread_mutex_init(&m_mutex, nullptr) != 0){
            throw std::exception();
        };
    };
    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    };
    bool lock(){
        /* pthread_mutex_lock阻塞函数，一直等待加锁 */
        return pthread_mutex_lock(&m_mutex) == 0;
    };
    bool unlock(){
        return pthread_mutex_unlock(&m_mutex) == 0;
    };
};

/* 条件变量，封装类 */
class cond{
private:
    pthread_cond_t m_cond;
public:
    cond(){
        if(pthread_cond_init(&m_cond, nullptr) != 0){
            throw std::exception(); /* 抛出异常 */
        };
    };
    ~cond(){
        pthread_cond_destroy(&m_cond);
    };
    /* 根据条件变量，阻塞线程 */
    bool wait(pthread_mutex_t* m_mutex){
        return pthread_cond_wait(&m_cond, m_mutex);
    }
    /* 唤起线程 */
    bool signal(){
        return pthread_cond_signal(&m_cond);
    }

};

/* 信号量，封装类 */
class sem{
private:
    sem_t m_sem;
public:
    /* 默认构造函数，无参数 */
    sem(){
        /* 第一个0代表，用于线程间，非0用在进程间 */
        if(sem_init(&m_sem, 0, 0) == -1){
            throw std::exception();
        }
    }
    /* 指定信号量num值 */
    sem(int num){
        if(sem_init(&m_sem, 0, num) == -1){
            throw std::exception();
        }
    }
    /* dtor */
    ~sem(){
        sem_destroy(&m_sem);
    }
    /* 信号量wait-1 */
    bool wait(){
        return sem_wait(&m_sem) == -1;  /* 调用一次对信号量值-1 */
    }
    /* 信号量post+1 */
    bool post(){
        return sem_post(&m_sem) == -1;
    }

};



#endif