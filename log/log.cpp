#include <sys/time.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "log.h"
using namespace std;

Log::Log(){
    m_count = 0;
    m_is_async = false;
}

Log::~Log(){
    if(m_fp != nullptr){
        fclose(m_fp);
    }
}

//默认输入参数，日志文件，日志缓冲区大小，最大行数以及最长日志队列
bool Log::init(const char* file_name, int log_buf_size, int split_lines, int max_queue_size){
    /* 如果设置max_queue_size，就是异步,生成阻塞队列 */
    if(max_queue_size >= 1){
        m_is_async = true;
        m_log_queue = new block_queue<string>(max_queue_size);
        /* 创建线程，异步写日志 （写到文件中） ，队列日志到文件 ，同步不需要，同步每次不经过队列，直接写入文件*/
        pthread_t tid;
        pthread_create(&tid, nullptr, flush_log_thread, nullptr);
    }

    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];   /* 建立日志缓冲区，char数组 */
    memset(m_buf, '\0', m_log_buf_size);   /* 清空缓冲区,头文件是string */
    m_split_lines = split_lines;

    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    const char* p = strrchr(file_name, '/');    /*从左向右在字符串中寻找字符C出现的最后一次出现的位置，并返回指向该位置的指针*/
    char log_full_name[256] = {0};  /* 完整的日志文件名 */

    if(p == nullptr){
        //没有路径分隔符，不需要分割路径和文件名
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }else{
        //有路径分隔符，分割路径和文件名
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name); 
    }
    m_today = my_tm.tm_mday;
    m_fp = fopen(log_full_name, "a");   /* 以追加方式打开文件，没有则新建 */
    if(m_fp == nullptr){
        return false;
    }
    return true;
}

void Log::write_log(int level, const char* format, ...){
    /* 获取当前时间 */
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    /* 获取系统时间 */
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch(level){
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[erro]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }
    m_mutex.lock(); /* 锁的是文件符 */
    m_count++;
    if(m_today != my_tm.tm_mday || m_count % m_split_lines == 0){
        /* 新的一天， 或者超过最大行数了，新建一个日志文件 */
        char new_log[]= {0};
        fflush(m_fp);
        fclose(m_fp);   /* 一定关闭上一个打开的文件 */
        char tail[16]= {0};
        
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
        /* 新的一天 */
        if(m_today != my_tm.tm_mday){
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }else{
            /* 同一天，新的日志文件，行数满了 */
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }
    m_mutex.unlock();

    /* 日志记录格式 */
    va_list valst;
    va_start(valst, format);
    string log_str;
    m_mutex.lock(); /* 锁m_buf缓冲区 */
    //每条日志前，具体的时间格式,写入m_buf缓冲区
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s", my_tm.tm_year + 1900,
                    my_tm.tm_mon + 1, my_tm.tm_mday, my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    /* 跟在时间后面，写入日志内容 */
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[m + n] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;
    m_mutex.unlock();

    if(m_is_async && !m_log_queue->full()){
        m_log_queue->push(log_str);
    }else{
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }
    
    va_end(valst);
}

void Log::flush(void){
    m_mutex.lock();
    //强制刷新写入流缓冲区
    fflush(m_fp);
    m_mutex.unlock();
}












