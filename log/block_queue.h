#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <stdlib.h>
#include "../locker/locker.h"
#include <sys/time.h>
using namespace std;


//模板类，声明和实现都在H文件中
template<typename T>
class block_queue{
public:
    block_queue(int max_size) : m_max_size(m_max_size){
        if(max_size <= 0){
            exit(-1);
        }
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_array = new T[m_max_size];
    };

    ~block_queue(){
        //操作循环数组，必须加锁
        m_mutex.lock();
        if(m_array != nullptr){
            delete []m_array;
        }
        m_mutex.unlock();
    }

    void clear(){
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }

    bool full(){
        m_mutex.lock();
        if(m_size >= m_max_size){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    bool empty(){
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }

    //返回队首元素
    bool front(T &value){
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();//一定记得要解锁！！！！
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }

    //返回队尾元素
    bool back(T &value){
        m_mutex.lock();
        if(m_size == 0){
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }

    int size(){
        int tmp = 0;
        m_mutex.lock();
        tmp = m_size;
        m_mutex.unlock();
        return tmp;
    }

    int max_size(){
        int tmp = 0;
        m_mutex.lock();
        tmp = m_max_size;
        m_mutex.unlock();
        return tmp;
    }

    /*
        往队列里添加元素，循环数组；
        当有元素push进队列，相当于生产者生产了一个元素，
        如果当前没有线程等待条件变量，则无意义
    */
    bool push(const T&item){
        m_mutex.lock();
        if(m_size >= m_max_size){
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        m_back = (m_back + 1) % m_max_size;
        m_size++;
        m_array[m_back] = item;
        m_cond.broadcast();//signal信号通知
        m_mutex.unlock();
        return true;
    }

    //pop时,如果当前队列没有元素,将会等待条件变量
    bool pop(T&item){
        m_mutex.lock();
        while(m_size <= 0){
            if(!m_cond.wait(m_mutex.get())){
                m_mutex.unlock();
                return false;
            }  
        }
        m_front = (m_front + 1) % m_max_size;
        m_size--;
        item = m_array[m_front];//最新的队首元素
        m_mutex.unlock();
        return true;
    }

    //增加了超时处理,上一个会一直循环wait，造成阻塞
    bool pop(T& item, int ms_timeout){
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, nullptr);
        m_mutex.lock();
        if(m_size <= 0){
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if(!m_cond.timewait(m_mutex.get(), t)){
                m_mutex.unlock();
                return false;
            }
        }
        //时间到，还未获得
        if(m_size <= 0){
            m_mutex.unlock();
            return false;
        }
        m_front = (m_front + 1) % m_max_size;
        m_size--;
        item = m_array[m_front];//返回新的队首元素
        m_mutex.unlock();
        return true;
    }

private:
    locker m_mutex;//自封装锁
    cond m_cond;//条件变量，线程间通信（通过信号）

    //循环数组实现阻塞队列
    T* m_array;
    int m_max_size;
    int m_size;
    int m_front;//记录头坐标
    int m_back;//记录尾坐标
};



#endif