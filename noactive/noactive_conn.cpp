#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#define MAX_EVENT_NUMBER 10000 /* 监听的最大事件数量 */

int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK; 
    fcntl(fd, F_SETFL, old_option);
    return old_option;
}

void addfd(int epollfd, int fd){
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    event.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd); /* ET一定要文件描述符fd非阻塞 */
}

void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void addsig(int sig, void(handler)(int)){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    /*
        sa.sa_mask是捕捉信号时的临时阻塞信号集
        sigfillset，将信号集中所有标志位置一
    */
    sigfillset(&sa.sa_mask);    /* 初始化阻塞所有信号 */
    sigaction(sig, &sa, nullptr);   /* 信号捕捉函数 */
}

/* main function */
int main(int argc, char* argv[]){
    if(argc <= 1){  /* check argment */
        printf("uasge: %s port_name\n", argv[0]);
        return -1;
    }
    int port = atoi(argv[1]);   /* 字符串转换为整数 */

    /*
        此处，为提前设计，避免当客户端关闭连接，服务端继续写，触发信号SIGPIPE，意外终止信号
        SIGPIPE,向一个没有读端的管道写数据SIG+_IGN, 忽略该信号
    */
   addsig(SIGPIPE, SIG_IGN);






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
    int ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reusem sizeof(reuse));
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
    int epollfd = epoll_create(8);
    if(epollfd == -1){
        perror("epoll_create");
        return -1;
    }
    addfd(epollfd, sockfd);
    http::epollfd = epollfd;    /* 静态变量，所有对象共享 */

    while(true){
        int num = epoll_wait(epollfd, &events, MAX_EVENT_NUMBER, -1); /* 此处events是传出参数！！！*/
        if((num < 0) && (errno != EINTR)){
            perror("epoll_wait");
            break;
        }
        for(int i = 0; i < num; i++){
            int curfd = events
        }
    }


    
















    return 0;
}