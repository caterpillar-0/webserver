#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

//http_conn是任务类，线程的执行函数http_conn->run()
class http_conn
{
public:
    //所有的socket事件被注册到同一个epoll内核中，设置为静态
    static int m_epollfd;
    //统计用户数量
    static int m_user_count;

public:
    http_conn(){};
    ~http_conn(){};

    void process();//处理客户端请求
    //初始化新接收的连接
    void init(int sockfd, const sockaddr_in &caddr);
    void close_conn();
    bool read();
    bool write();

private:
    int m_sockfd;//该http连接的socket
    sockaddr_in m_address;//通信的socket地址

};

#endif