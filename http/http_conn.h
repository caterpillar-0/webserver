#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

//http_conn是任务类，线程的执行函数http_conn->run()
class http_conn
{
public:
    /*
        static member variable
    */

    static int m_epollfd;   /* 所有的socket事件被注册到同一个epoll内核中，设置为静态 */
    static int m_user_count;    /* 统计用户数量 */

    static const int READ_BUFFER_SIZE = 2048;//define
    static const int WRITE_BUFFER_SIZE = 2048;

    /*
        服务器处理HTTP请求的可能结果，报文解析的结果
        NO_REQUEST          :   请求不完整，需要继续读取客户数据
        GET_REQUEST         :   表示获得了一个完成的客户请求
        BAD_REQUEST         :   表示客户请求语法错误
        NO_RESOURCE         :   表示服务器没有资源
        FORBIDDEN_REQUEST   :   表示客户对资源没有足够的访问权限
        FILE_REQUEST        :   文件请求,获取文件成功
        INTERNAL_ERROR      :   表示服务器内部错误
        CLOSED_CONNECTION   :   表示客户端已经关闭连接了
    */
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    enum METHED {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};    /* HTTP request method enumerate */
    enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT};    /* Master state machine */
    enum LINE_STATE {LINE_OK = 0, LINE_BAD, LINE_OPEN}; /* Secondary state machine */

public:
    /*
        public member function
    */
    http_conn(){};
    ~http_conn(){};

    void process();     /* deal client request */
    void init(int sockfd, const sockaddr_in &caddr);    /* init new connect socket */
    void close_conn();
    bool read();    /*non-blocking read */
    bool write();   /* non-blocking write */

private:
    /*
        private member function
    */
   void init();
   HTTP_CODE process_read();    /* parse http reqeust*/
   
   HTTP_CODE parse_request_line(char* text);
   HTTP_CODE parse_headers(char* text);
   HTTP_CODE parse_content(char* text);
   LINE_STATE parse_line();
   char* get_line(){return m_read_buf + m_start_line};

private:
    /*
        private member variables
    */
    int m_sockfd;   /* 该http连接的socket */
    sockaddr_in m_address;  /* 通信的socket地址 */
    int m_read_idx;     /* 记录已读数据缓冲区的下一个位置 */
    char m_read_buf[READ_BUFFER_SIZE];  /* read buffer */

    CHECK_STATE m_check_state;  /*master machine cur state */
    int m_checked_idx;  /* parse char index in read buffer */
    int m_start_line;   /* parse line index in read buffer */



};

#endif