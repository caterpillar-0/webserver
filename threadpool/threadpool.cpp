#include "../threadpool/threadpool.h"

//模板类成员函数实现
//ctor
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
    }
};


//dtor
template <typename T>
threadpool<T>::~threadpool(){
    delete []m_threads;
}

//append
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
    threadpool pool = (threadpool*)arg;
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
    }
    return;
}