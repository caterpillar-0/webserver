#include "http_conn.h"

//-----------------------extern function------------------

//设置文件描述符非阻塞
int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFD);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFD, new_option);
    return old_option;
}

//向epoll中添加要监听的文件描述符
void addfd(int epollfd, int fd, bool one_shot){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}

//从epoll中移除fd
void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//--------------static member variable init-----------
//所有的socket事件被注册到同一个epoll内核中，所有客户连接对象共用一个，因此为static静态
int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;
//当前用户连接数


//-------------------member function----------------------------
//处理客户端请求
void http_conn::process(){
    printf("process....\n");
}

//初始化新接收的连接
void http_conn::init(int sockfd, const sockaddr_in &caddr){
    m_sockfd = sockfd;
    m_address = caddr;
    //set port reuse
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    addfd(m_epollfd, m_sockfd, true);
    m_user_count++;
}

//关闭连接
void http_conn::close_conn(){
    if(m_sockfd != -1){
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;//关闭一个连接，将客户总数-1
    }
}
//读数据
bool http_conn::read(){
    printf("read\n");
    return true;
}
//写数据
bool http_conn::write(){
    printf("write\n");
    return true;
}





