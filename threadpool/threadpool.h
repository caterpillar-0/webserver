#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <list>
#include "../locker/locker.h"

//定义线程池类,模板类，任务不同也可以复用
template <typename T>
class threadpool
{
private:
    int m_pthread_num;//线程池数目
    pthread_t* m_threads;//线程池数组

    int m_max_request;//最大任务数目
    std::list<T*> m_workque;//任务队列
    locker m_quelocker;//保护队列的互斥锁
    sem m_quesem;//保护队列的信号量

    bool m_stop;

private:
    //工作线程运行的函数，不断从任务队列中取出任务执行
    //注意，静态函数，无法直接访问类成员，因此，此处传入*this
    static void* worker(void* arg);
    void run();

public:
    threadpool(int pthread_num = 8, int max_request = 10000);
    ~threadpool();
    bool append(T* request);//添加任务到任务队列
};

#endif