#include <stdlib.h>
#include <signal.h>
#include "threadpool/threadpool.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <string.h>
#include <sys/types.h>
#include "http/http_conn.h"
#include <stdio.h>
#include <errno.h>  
#include "locker/locker.h"
#include <exception>
#include <errno.h>

#define MAX_FD 65535 /* 最大的文件描述符数目 */
#define MAX_EVENT_NUMBER 10000 /* 监听的最大事件数量 */

extern void addfd(int epollfd, int fd, bool one_shot);  /* 添加fd到epoll */
extern void removefd(int epollfd, int fd);

void addsig(int sig, void(handler)(int)){
    /* handlaer信号处理函数指针，此处选择忽略，SIG_IGN告诉系统什么也不做 */
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    /*
        sa.sa_mask是捕捉信号时的临时阻塞信号集
        sigfillset，将信号集中所有标志位置一
    */
    sigfillset(&sa.sa_mask);    /* 初始化阻塞所有信号 */
    sigaction(sig, &sa, nullptr);   /* 信号捕捉函数，检查或者改变信号的处理 */
}

int main(int argc, char* argv[]){
    /* ./server 9999 端口号 */
    if(argc <= 1){
        printf("usage: %s port_number\n", basename(argv[0]));
        return 1;
    }

    int port = atoi(argv[1]);
    /*
        此处，为提前设计，避免当客户端关闭连接，服务端继续写，触发信号SIGPIPE，意外终止信号
        SIGPIPE,向一个没有读端的管道写数据
        SIG+_IGN, 忽略该信号
    */
    addsig(SIGPIPE, SIG_IGN);
    
    /* 初始化线程池 */
    threadpool<http_conn> *pool = nullptr;
    try{
        pool = new threadpool<http_conn>;
    } catch( ... ){
        return 1;
    }

    http_conn* users = new http_conn[MAX_FD];
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){
        perror("socket");
        return -1;
    }

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;

    int reuse = 1;  /* 端口复用 */
    int ret;
    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if(ret == -1){
        perror("setsockopt");
        return -1;
    }
    ret = bind(listenfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if(ret == -1){
        perror("bind");
        return -1;
    }
    ret = listen(listenfd, 5);
    if(ret == -1){
        perror("listen");
        return -1;
    }

    /* 设置epoll,epoll底层是红黑树和双向链表，其中epoll_wait返回传出参数 */
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);      /* 随便一个数即可，无意义 */
    addfd(epollfd, listenfd, false);    /* 添加fd到epoll */
    http_conn:: m_epollfd = epollfd;

    while(true){
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1); /* 此处，events是传出参数*/

        /* ENTR The call was interupted by a signal handler */
        if((number < 0) && (errno != EINTR)){
            printf("epoll_wait failure\n");     
            break;
        }

        /* 遍历监听有事件的fd */
        for(int i = 0; i < number; i++){
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd){ 
                /* new client connection */
                struct sockaddr_in caddr;
                socklen_t len = sizeof(caddr);
                int connfd = accept(sockfd, (struct sockaddr*)&caddr, &len);
                if(connfd < 0){
                    printf("errno is : %d\n", errno);
                    continue;
                }
                if(http_conn::m_user_count >= MAX_FD){
                    close(connfd);
                    continue;
                }
                users[connfd].init(connfd, caddr);
                
            }else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                /*
                    EPOLLRDHUP-----Stream socket peer closed connection, or shut down writing half of connection.
                    EPOLLERR------Error condition happened on the associated file descriptor.
                    EPOLLHUP-----Hang up happened on the associated file descriptor.
                */
                users[sockfd].close_conn();
                printf("--close_conn--\n");
            }else if(events[i].events & EPOLLIN){
                printf("-----EPOLLIN----\n");
                if(users[sockfd].read()){
                    if(pool->append(users + sockfd) == false){   /* 主线程完成读数据，交给任务队列，业务逻辑 */
                        printf("append failed!\n");
                        return -1;
                    }
                }else{
                    printf("read return false\n");
                    users[sockfd].close_conn(); /* 读完数据，关闭连接 */
                }
            }else if(events[i].events & EPOLLOUT){
                printf("write!\n");
                if(!users[sockfd].write()){
                    users[sockfd].close_conn();
                }
            }
        }
    }
    close(epollfd);
    close(listenfd);
    delete pool;
    return 0;
}