# include "lst_timer.h"

//public函数实现
sort_timer_lst::~sort_timer_lst(){
    while(head != tail){
        util_timer* cur = head;
        head = head->next;
        delete cur;
    }   
}

void sort_timer_lst::add_timer(util_timer * timer){
    if(!timer){
        return;
    }
    if(!head){
        head = tail = timer;
        return;
    }
    //时间都小于链表定时器，头部插入
    if(timer->expire < head->expire){
        timer->next = head->next;
        head->prev = timer;
        head = timer;
        return;
    }
    //使用私有函数，从head开始寻找合适位置插入
    add_timer(timer, head);
}

//当某个定时器定时事件加长，调整其位置，目前只考虑加长时间
//头部和尾部节点调整，都要额外处理
void sort_timer_lst::adjust_timer(util_timer* timer){
    if(!timer){
        return;
    }
    util_timer* tmp = timer->next;
    //timer是尾部节点，或者加长时间后仍然小于下一个时间，不动，直接返回
    if(!tmp || (timer->expire < tmp->expire)){
        return;
    }
    //取出在重新插入
    if(head == timer){
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer, head);
    }else{
        timer->next->prev = timer->prev;
        timer->prev->next = timer->next;
        add_timer(timer, timer->next);
    }
    return;
}

//删除某一指定定时器
void sort_timer_lst::del_timer(util_timer* timer){
    //为了避免空指针操作，要考虑特殊情况，至少timer要存在且存在前后定时器
    // timer->prev->next = timer->next;
    // timer->next->prev = timer->prev;
    // delete timer;

    if(!timer){
        return;
    }
    if(!head){
        return;
    }
    //单节点
    if(head == tail && head == timer){
        delete head;
        head = nullptr;
        tail = nullptr;
        return;
    }
    //以下至少两个
    //处理头结点timer
    if(timer == head){
        head = head->next;
        head->prev = nullptr;
        delete timer;
        return;
    }
    //处理尾结点timer
    if(timer == tail){
        tail = tail->prev;
        tail->next = nullptr;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}
void sort_timer_lst::tick(){
    //脉搏函数，定时事件到，开始删除过期定时器
    if(!head){
        return;
    }
    printf("lst_timer.cpp : 100, tick()!\n");
    time_t cur = time(nullptr);//获取当前系统时间，绝对数值
    util_timer* tmp = head;
    while(tmp){
        if(tmp->expire > cur){
            break;
        }
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if(head){
            head->prev = nullptr;
        }
        delete tmp;
        tmp = head;
    }
    return;
}

//private函数实现
void sort_timer_lst::add_timer(util_timer* timer, util_timer* lst_head){
    //从lst_head开始，寻找合适位置插入
    util_timer* tmp = lst_head;
    while(tmp){
        if(timer->expire < tmp->expire){
            util_timer* pre = tmp->prev;
            pre->next = timer;
            timer->prev = pre;
            timer->next = tmp;
            tmp->prev = timer;
        }
        tmp = tmp->next;
    }
    //大于所有定时超时时间，插入末尾
    tail->next = timer;
    timer->prev = tail;
    timer->next = nullptr;
    tail = timer;
}






