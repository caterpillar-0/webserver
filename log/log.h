#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string>
#include "block_queue.h"
#include "../locker/locker.h"
using namespace std;

#define MAX_NAME_SIZE 128

/* 同步/异步日志系统，加入阻塞队列主要是为了解决异步写入日志做准备 */
class Log{
public:
    static Log* get_instance(){
        static Log instance;    /* static局部变量，第一次调用初始化，到程序结束 */
        return &instance;
    }
    static void* flush_log_thread(void* args){
        Log::get_instance()->async_write_log();
    }
    //默认输入参数，日志文件，日志缓冲区大小，最大行数以及最长日志队列
    bool init(const char* file_name, int log_buf_size = 8192, int split_lines = 500000, int max_queue_size = 0);
    void write_log(int level, const char* format, ...); /* 可选参数 */
    void flush(void);
private:
    /* 将ctor&dtor私有化，保证单例，全局只有一个Log实例，只能通过static函数get_instance */
    Log();
    ~Log();
    void async_write_log(){
        /* 队列到日志文件 */
        string single_log;
        while(m_log_queue->pop(single_log)){
            m_mutex.lock(); /* 锁的是m_fp ,保证对文件的互斥访问 */
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

private:
    /* 避免魔法数字 */
    char dir_name[MAX_NAME_SIZE];   /* 路径名 */
    char log_name[MAX_NAME_SIZE];    /* 日志名 */
    int m_split_lines;  /* 最大行数 */
    int m_log_buf_size; /* 日志缓冲区大小 */
    long long m_count;  /* 日志行数记录 */
    int m_today;    /* 按天分类，记录当前时间是哪一天 */
    FILE *m_fp; /* 打开log文件指针 */
    char *m_buf;    /* 缓冲区 */
    block_queue<string> *m_log_queue;   /* 阻塞队列 */
    bool m_is_async;    /* 是否异步标记 */
    /* sysnchronous-同步， asynchronous-异步 */
    locker m_mutex;
};

#define LOG_DEBUG(format, ...) Log::get_instance()->write_log(0, format, ##__VA__ARGS__);
#define LOG_INFO(format, ...) Log::get_instance()->write_log(1, format, ##__VA_ARGS__);
#define LOG_WARN(format, ...) Log::get_instance()->write_log(2, format, ##__VA_ARGS__);
#define LOG_ERROR(format, ...) Log::get_instance()->write_log(3, format, ##__VA_ARGS__);

#endif 