#ifndef LST_TIMER
#define LST_TIMER

#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>

class util_timer;//前向声明

struct client_data{
    sockaddr_in address;
    int sockfd;
    util_timer* timer;
};

/* 在类成员函数中定义的隐式内联函数 */
/* 定时器说明 */
class util_timer{
public:
    util_timer():prev(nullptr), next(nullptr){};
public:
    util_timer* prev;
    util_timer* next;
    time_t expire;
    void (*cb_func)(client_data*);
    client_data* user_data;
};

/* 定时器排序链表 */
class sort_timer_lst{
public:
    sort_timer_lst():head(nullptr), tail(nullptr){};
    ~sort_timer_lst();
    void add_timer(util_timer * timer);
    void adjust_timer(util_timer* timer);
    void del_timer(util_timer* timer);
    void tick();
    
private:
    void add_timer(util_timer* timer, util_timer* lst_head);
    
private:
    util_timer *head;
    util_timer *tail;
};

#endif