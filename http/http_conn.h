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
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <sys/uio.h>
#include <signal.h>
#include <netinet/in.h>
#include <pthread.h>
#include "../noactive/lst_timer.h"
#include "../CGImysql/sql_connection_pool.h"

class util_timer;   /* 前向声明 */

//http_conn是任务类，线程的执行函数http_conn->run()
class http_conn
{
public:
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 2048;
    static const int FILENAME_LEN = 200;    /* filename max length */

    enum HTTP_CODE { 
        NO_REQUEST,     /* 请求不完整，需要继续读取客户数据 */
        GET_REQUEST,    /* 表示获得了一个完成的客户请求 */
        BAD_REQUEST,    /* 表示客户请求语法错误 */
        NO_RESOURCE,    /* 表示服务器没有资源 */
        FORBIDDEN_REQUEST,  /* 表示客户对资源没有足够的访问权限 */
        FILE_REQUEST,       /* 文件请求,获取文件成功 */
        INTERNAL_ERROR,     /* 表示服务器内部错误 */
        CLOSED_CONNECTION   /* 表示客户端已经关闭连接了 */
    };

    /* HTTP request method enumerate */
    enum METHED {
        GET = 0, 
        POST, 
        HEAD, 
        PUT, 
        DELETE, 
        TRACE, 
        OPTIONS, 
        CONNECT
    };    

    /* Master state machine */
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0, 
        CHECK_STATE_HEADER, 
        CHECK_STATE_CONTENT
    };    

    /* Secondary state machine */
    enum LINE_STATE {
        LINE_OK = 0, 
        LINE_BAD, 
        LINE_OPEN
    }; 

public:
    http_conn(){};
    ~http_conn(){};

public:
    void init(int sockfd, const sockaddr_in &caddr);    /* init new connect socket */
    void close_conn(bool real_close = true);
    void process();     /* deal client request */
    bool read();    /* non-blocking read */
    bool write();   /* non-blocking write */

    void initmysql_result(connection_pool* connPool);

private:
    void init();

    /* parse http reqeust*/
    HTTP_CODE process_read();    
    LINE_STATE parse_line();
    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_headers(char* text);
    HTTP_CODE parse_content(char* text);
    HTTP_CODE do_request();
    char* get_line(){return m_read_buf + m_start_line;};

    /* response http message */
    bool process_write(HTTP_CODE ret);
    bool add_response( const char* format, ... );
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_length);
    bool add_content(const char* content);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
    void unmap();

public:
    static int m_epollfd;   /* 所有的socket事件被注册到同一个epoll内核中，设置为静态 */
    static int m_user_count;    /* 统计用户数量 */

private:
    int m_sockfd;   /* 该http连接的socket */
    sockaddr_in m_address;  /* 通信的socket地址 */
    int m_read_idx;     /* 记录已读数据缓冲区的下一个位置 */
    char m_read_buf[READ_BUFFER_SIZE];  /* read buffer */

    CHECK_STATE m_check_state;  /*master machine cur state */
    METHED m_method;
    int m_checked_idx;  /* parse char index in read buffer */
    int m_start_line;   /* parse line index in read buffer */
    char* m_url;    
    char* m_version;
    int m_content_length;   /* HTTP request message total length */
    bool m_linger;  /* wthether keep-alive */
    char* m_host;   /* request header: Host */

    char* m_file_address;    /* target file mmap memeory address */
    char m_real_file[ FILENAME_LEN ];   /* 客户请求的目标文件的完整路径，其内容等于 doc_root + m_url, doc_root是网站根目录 */
    struct stat m_file_stat;    /* 目标文件的状态。通过它我们可以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息 */
    struct iovec m_iv[2];   /* 我们将采用writev来执行写操作，所以定义下面两个成员，其中m_iv_count表示被写内存块的数量。 */
    int m_iv_count;

    char m_write_buf[WRITE_BUFFER_SIZE];    /* write buffer */
    int m_write_idx;    /* 写缓冲区中待发送的位置 */

    /* write bug fix add field */
    int bytes_to_send;  
    int bytes_have_send;    

    int cgi;    /* 是否启用POST */
    char* m_string;     /* 存储消息体数据 */
    MYSQL *mysql;
};

#endif