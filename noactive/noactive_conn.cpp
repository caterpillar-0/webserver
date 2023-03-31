#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "lst_timer.h"
#include <unistd.h>
#include <netinet/in.h>

#define MAX_EVENT_NUMBER 10000 /* 监听的最大事件数量 */
#define FD_LIMIT 65535
#define TIMESLOT 5

static int pipefd[2];   /* 管道，信号通知 */
static sort_timer_lst timer_lst;
static int epollfd = 0;

int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK; 
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd){
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    event.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd); /* ET一定要文件描述符fd非阻塞 */
}

void sig_handler(int sig){
    printf("main.cpp : 40, sig_handler\n");
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char*)&msg, 1, 0);  /* 将sig写入管道 */
    errno = save_errno;
}

void addsig(int sig){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    /*
        sa.sa_mask是捕捉信号时的临时阻塞信号集
        sigfillset，将信号集中所有标志位置一
    */
    sigfillset(&sa.sa_mask);    /* 初始化阻塞所有信号 */
    assert(sigaction(sig, &sa, nullptr) != -1);   /* 信号捕捉函数 */
}

/* 定时器的成员函数，定时器超时后的回调函数 */
void cb_func(http_conn* user_data){
    assert(user_data);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    close(user_data->sockfd);
    printf("main.cpp : 59, close fd %d\n", user_data->sockfd);
}

/* main function */
int main(int argc, char* argv[]){
    if(argc <= 1){  /* check argment */
        printf("uasge: %s port_name\n", argv[0]);
        return -1;
    }
    int port = atoi(argv[1]);   /* 字符串转换为整数 */

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){
        perror("socket");
        return -1;
    }
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);   /* 主机字节序转换为网络字节序 （大小端）*/
    saddr.sin_addr.s_addr = INADDR_ANY;

    int reuse = 1;  /* 设置端口复用，在bind之前才可以 */
    int ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if(ret == -1){
        perror("setsockopt");
        return -1;
    }
    ret = bind(listenfd, (const sockaddr*)&saddr, sizeof(saddr));
    if(ret == -1){
        perror("bind");
        return -1;
    }
    ret = listen(listenfd, 5);
    if(ret == -1){
        perror("listen");
        return -1;
    }
    
    struct epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    if(epollfd == -1){
        perror("epoll_create");
        return -1;
    }
    addfd(epollfd, listenfd);

    /* 创建全双工管道 ，pipe[0]从管道读， Pipe[1]写入管道 */
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    if(ret == -1){
        perror("socketpair");
        return -1;
    }
    setnonblocking(pipefd[1]);  /* 设置写端非阻塞，管道缓冲区满时，不会一直阻塞等待*/
    addfd(epollfd, pipefd[0]);  /* 读端加入epoll事件监听 */

    /* 加入信号捕获，一旦捕获执行sig_handler, 写入pipefd[1], 触发pipefd[0]读事件 */
    addsig(SIGALRM);    /* 定时器到时 */
    addsig(SIGTERM);    /* 优雅关闭 */

    bool stop_server = false;
    client_data* users = new client_data[FD_LIMIT];
    bool timeout = false;
    alarm(TIMESLOT);    /* 开启定时器 */

    while(!stop_server){
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1); /* 此处events是传出参数！！！*/
        if((num < 0) && (errno != EINTR)){
            /* EINTR是慢系统调用阻塞时被中断发出的错误码 ，continue即可，不然就会abort()直接退出 */
            perror("epoll_wait");   
            break;
        }
        for(int i = 0; i < num; i++){
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd){ /* 新的客户端连接请求 */
                struct sockaddr_in caddr;
                socklen_t len = sizeof(caddr);
                int connfd = accept(listenfd, (struct sockaddr*)&caddr, &len);
                addfd(epollfd, connfd);
                users[connfd].caddr = caddr;
                users[connfd].sockfd = connfd;

                /* 创建定时器，绑定定时器与用户数据，将定时器添加进排序链表中 */
                util_timer* timer = new util_timer;
                timer->user_data = &users[connfd];
                timer->cb_func = cb_func;
                timer->expire = time(nullptr) + 3 * TIMESLOT;
                users[connfd].timer = timer;
                timer_lst.add_timer(timer);
            }else if((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)){
                /* 有定时器超时触发SIGARM信号，往管道写数据，触发EPOLLIN检测写事件 */
                /* 处理sig信号(从管道中读取信号信息) */
                printf("main.cpp:155, pipefd[0]\n");
                int sig;
                char signals[1024]; /* 一个信号，一个char */
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if(ret == -1 || ret == 0){
                    continue;
                }else{
                    for(int i = 0; i < ret; i++){
                        switch (signals[i]){
                        case SIGALRM:
                            timeout = true; /* 优先级不高，timeout标记一下 */
                            break;
                        case SIGTERM:
                            stop_server = true; /* 优雅关闭 */
                        default:
                            break;
                        }
                    }
                }
            }else if(events[i].events & EPOLLIN){
                /* 客户端发送数据过来了 */
                memset(users[sockfd].buf, '\0', BUFFER_SIZE);
                ret = recv(sockfd, users[sockfd].buf, BUFFER_SIZE - 1, 0);
                printf("noactive_conn.cpp : 173, get %d bytes of client data %s from %d\n", ret, users[sockfd].buf, sockfd );
                util_timer* timer = users[sockfd].timer;
                if(((ret < 0) && (errno != EAGAIN)) || ret == 0){
                    /* EAGAIN错误码只是数据读完了，不是真的错误 */
                    cb_func(sockfd);
                    if(timer){
                        timer_lst.del_timer(timer);
                    }
                }else{
                    /* 读取到数据 ，调整定时器时间 */
                    if(timer){
                        timer->expire = time(nullptr) + 3 * TIMESLOT;
                        printf("noactive_conn.cpp : 184, adjust timer once\n");
                        timer_lst.adjust_timer(timer);
                    }
                }
            
            }
        }
        if(timeout){
            timer_lst.tick();    /* 调用tick函数 */
            alarm(TIMESLOT);    /* 开启定时器 */
            timeout = false;
        }
    }
    close(listenfd);
    close(pipefd[0]);
    close(pipefd[1]);
    delete []users;
    return 0;
}