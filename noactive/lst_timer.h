#ifndef LST_TIMER
#define LST_TIMER

#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 64
class util_timer; /* 前向声明 */

/* 给每个连接，配一个定时器 ，使用链表连接 */

/* 用户数据结构 */
struct client_data
{
    sockaddr_in caddr;
    int sockfd;
    char buf[BUFFER_SIZE];  /* 读缓存 */
    util_timer* timer;  /* 定时器 */
};

/* 定时器类---双向链表节点 */
class util_timer{
public:
    util_timer() : prev(nullptr), next(nullptr){}

public:
    util_timer* prev; 
    util_timer* next;
    client_data* user_data;
    time_t expire;  /*  任务超时时间， 使用绝对时间 */
    /* 任务回调函数，回调函数处理客户数据，由定时器执行者传递给回调函数 */
    void (*cb_func)(client_data*);  
};

/* 定时器链表， 一个升序、双向链表，且带有头结点和尾节点 */
class sort_timer_lst{
public:
    sort_timer_lst() : head(nullptr), tail(nullptr){}
    ~sort_timer_lst(){
        util_timer* cur = head;
        while(cur){
            util_timer* tmp = cur;
            cur = cur->next;
            delete tmp;
        }
    }

    /* add timer into list */
    void add_timer(util_timer* timer){
        if(!timer){ /* timer空指针检验 */
            return;
        }
        if(!head){  /* head空指针 */
            head = tail = timer;
            return;
        }
        if(timer->expire < head->expire){   /* 如果目标定时器的超时时间小于当前链表中所有定时器的超时时间，则将定时器插入链表头部， */
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        move_timer(timer, head);   /* 否则mv_timer，把它插入到合适位置，保证链表的有序性 */
    }

    /* 
        当某个定时任务发生变化，调整对应定时器在链表中的位置，
        这个函数只考虑被调整的定时器的超时时间延长的情况， 即该定时器需要往链表尾部移动 
    */
    void adjust_timer(util_timer* timer){
        if(!timer){
            return;
        }
        util_timer* tmp = timer->next;
        if(!tmp || (timer->expire < tmp->expire)){
            return; /* 如果目标timer在尾部，或者下一个timer就已经满足条件，无需调整 */
        }
        if(timer == head){  /* 目标timer是头结点 */
            head = head->next;
            head->prev = nullptr;
            timer->next = nullptr;
            move_timer(timer, head);
        }else{
            // 如果目标定时器不是链表的头节点，则将该定时器从链表中取出，然后插入其原来所在位置后的部分链表中
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            move_timer( timer, timer->next );
        }

    }

    /* delete timer in list */
    void del_timer(util_timer* timer){
        if(!timer){
            return; /* 非空指针判断 */
        }
        if(head == timer && tail == timer){ /*  链表只有一个定时器 */
            delete timer;
            head = nullptr;
            tail = nullptr;
            return;
        }
        if(timer == head){  /* timer是头结点 */
            head = head->next;
            head->prev = nullptr;
            delete timer;
            return;
        }
        if(timer == tail){  /* timer是尾结点 */
            tail = tail->prev;
            tail->next = nullptr;
            delete timer;
            return;
        }
        /* timer为链表中间某个位置 */
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }

    /* SIGALRM信号每次触发就在其信号处理函数中执行一次tick()函数，以处理链表上的到期任务 */
    void tick(){
        if(!head){
            return;
        }
        printf("lst_timer.h : 96 , timer tick/n");
        time_t cur = time(nullptr); /* get system timer */
        util_timer* node = head;
        /* 遍历节点，直到遇到一个尚未到期的定时器 */
        while(node){
            if(node->expire > cur){ /* 采用绝对时间，与系统时间直接相比 */
                break;
            }
            node->cb_func(node->user_data); /* 调用定时器的回调函数，执行定时任务 */
            head = node->next;  /* 删除当前定时器节点 */
            if(head){
                head->prev = nullptr;   /* 一定要防止空指针操作 */
            }
            delete node;
            node = head;
        }
    }
    
private:
    /* 
        一个重载的辅助函数，被公有的add_timer函数和adjust_timer函数调用，
        该函数表示将目标定时器timer添加到节点Lst_head之后部分链表中；
    */
    void move_timer(util_timer* timer, util_timer* lst_head){
        util_timer* pre = lst_head;
        util_timer* cur = lst_head->next;
        
        while(cur){ /* 遍历lst_head节点之后部分链表，寻找合适位置，插入 */
            if(cur->expire > timer->expire){
                pre->next = timer;
                timer->prev = pre;
                timer->next = cur;
                cur->prev = timer;
                break;
            }
            pre = cur;
            cur = cur->next;
        }
        if(!cur){   /* 遍历到尾结点之后，直接插入尾节点 */
            tail->next = timer;
            timer->prev = tail;
            timer->next = nullptr;
            tail = timer;
        }
    }



private:
    util_timer* head;   
    util_timer* tail;

};














#endif