#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <list>
#include <stdio.h>
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
/*
    工作线程运行的函数，不断从任务队列中取出任务执行
    注意，静态函数，无法直接访问类成员，因此，此处传入*this
*/
    static void* worker(void* arg);
    void run();

public:
    threadpool(int pthread_num = 8, int max_request = 10000);
    ~threadpool();
    bool append(T* request);//添加任务到任务队列
};


//-----------------------template class number function-------------------
//------------ctor------------------
template <typename T>
threadpool<T>::threadpool(int pthread_num, int max_request):
m_pthread_num(pthread_num), m_max_request(max_request), 
m_stop(false)
{
    //check argument
    if(pthread_num <= 0 || max_request <= 0){
        throw std::exception();
    }
    //new m_pthreads, note delete[]!
    m_threads = new pthread_t[m_pthread_num];
    if(!m_threads){
        throw std::exception();
    }
    //m_pthreads
    for(int i = 0; i < m_pthread_num; i++){
        //create pthread
        if(pthread_create(m_threads + i, nullptr, worker, this) != 0){
            delete []m_threads;
            throw std::exception();
        }
        //detach pthread
        if(pthread_detach(m_threads[i]) != 0){
            delete []m_threads;
            throw std::exception();
        }
        printf("create thread %d\n", i);
    }
};


//-----------dtor--------------
template <typename T>
threadpool<T>::~threadpool(){
    delete []m_threads;
}

//----------------append-----------------
template <typename T>
bool threadpool<T>::append(T* request){
    m_quelocker.lock();
    //检查队列是否满
    if(request == nullptr || m_workque.size() >= m_max_request){
        m_quelocker.unlock();//记得解锁
        return false;
    }
    //将任务推入队列,注意，多线程操作队列，需要加锁
    m_workque.push_back(request);
    m_quelocker.unlock();
    m_quesem.post();
    return true;
}

template <typename T>
void *threadpool<T>:: worker(void* arg){
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run(){
    //从任务队列中取任务执行
    while(true){
        m_quesem.wait();//阻塞等待唤醒
        m_quelocker.lock();
        if(m_workque.empty()){
            m_quelocker.unlock();
            continue;
        }
        T* request = m_workque.front();
        m_workque.pop_front();
        request->process();//调用任务函数
        printf("processing...\n");
    }
    return;
}

#endif