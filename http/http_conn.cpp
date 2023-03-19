#include "http_conn.h"

/*
    define && const variables
*/

/* file resource root address */
const char* doc_root = "/home/xmm/code/webserver3/webserver";

/* 定义HTTP响应的一些状态信息 */
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";


/*
    extern function
    operation of fd(add, remove, modify) to epollfd
*/

/* set fd non-blocking */
int setnonblocking(int fd){     
    int old_option = fcntl(fd, F_GETFD);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFD, new_option);
    return old_option;
}

/*
    reset EPOLLONESHOT events on fd, to ensure triggered next time
*/
void modfd(int epollfd, int fd, int ev){
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, 0);
}

void addfd(int epollfd, int fd, bool one_shot){
    epoll_event event;
    event.data.fd = fd;
    //event.events = EPOLLIN | EPOLLRDHUP;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    if(one_shot) 
    {
        event.events |= EPOLLONESHOT;   /* 防止同一个通信被不同的线程处理 */
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);     /* set fd non-blocking */
}

void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

/*
    static member variable init
*/

int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;

/*
    private member function
*/

/* init private info connection */
void http_conn::init(){
    m_check_state = CHECK_STATE_REQUESTLINE;

    m_start_line = 0;
    m_read_idx = 0;
    m_version = 0;
    m_url = 0;
    m_method = GET;
    bzero(m_read_buf, READ_BUFFER_SIZE);    /*clear read_buf */

}

/* parse one line in http request ,flag is '\r\n' */
http_conn::LINE_STATE http_conn::parse_line(){
    char temp;
    for(; m_checked_idx < m_read_idx; ++m_checked_idx){
        temp = m_read_buf[m_checked_idx];
        if(temp == '\r'){
            if((m_checked_idx + 1) == m_read_idx){
                return LINE_OPEN;
            }else if(m_read_buf[m_checked_idx + 1] == '\n'){
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }else if(temp == '\n'){
            if((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\n')){
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

/* parse http reqeust, master machine convert */
http_conn::HTTP_CODE http_conn::process_read(){
    LINE_STATE line_status = LINE_OK;   /* secondary status */
    HTTP_CODE ret = NO_REQUEST;     /* http response */
    char* text = 0;
    /* parse content don't line by line, addtiton judgement*/
    while((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || 
    ((line_status = parse_line()) == LINE_OK)){     
        text = get_line();
        m_start_line = m_checked_idx;
        printf("got one line: %s\n", text);
        /* Master machine convert */
        switch(m_check_state){
            case CHECK_STATE_REQUESTLINE: {
                ret = parse_request_line(text);
                if(ret == BAD_REQUEST){
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER: {
                ret = parse_headers(text);
                if(ret == BAD_REQUEST){
                    return BAD_REQUEST;
                }else if(ret == GET_REQUEST){
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT: {
                ret = parse_content(text);
                if(ret == GET_REQUEST){
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            }
            default:{
                return INTERNAL_ERROR;
            }
        }
    }
    return NO_REQUEST; 
}   

/* parse headers, get method, file name, version*/
http_conn::HTTP_CODE http_conn::parse_request_line(char* text){
    /* get method */
    m_url = strpbrk(text, "\t"); /* GET /index.html HTTP/1.1 */
    if(!m_url){
        return BAD_REQUEST;
    }
    *m_url++ = '\0';    /* GET\0/index.html HTTP/1.1 */
    char* method = text;
    if(strcasecmp(method, "GET") == 0){
        m_method = GET;
    }else{
        return BAD_REQUEST;
    }
    /* get m_version */
    m_version = strpbrk(m_url, "\t");
    if(!m_version){
        return BAD_REQUEST;
    }
    *m_version++ = '\0';    /* /index.html\0HTTP/1.1 */
    if(strcmp(m_version, "HTTP/1.1") != 0){
        return BAD_REQUEST;
    }
    /* get m_url */
    if(strncasecmp(m_url, "http://", 7) == 0){
        m_url += 7;     /* http://192.168.110.129:10000/index.html */
        m_url = strchr(m_url, '/');     /* 192.168.110.129:10000/index.html */ 
    }
    if(!m_url || m_url[0] != '/'){
        return BAD_REQUEST;
    }
    m_check_state = CHECK_STATE_REQUESTLINE;    /* convert master machine status */
    return NO_REQUEST;
}

/* parse one header*/
http_conn::HTTP_CODE http_conn::parse_headers(char* text){
    /* check empty line, headers already parse end*/
    if(text[0] == '\0'){
        if(m_content_length != 0){
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;  /* continue to parse content */
        }
        return GET_REQUEST; /* already parse complete request */
    }else if(strncasecmp(text, "Connection:", 11) == 0){
        /* Connection : keep-alive ,tcp保活机制 */
        text += 11;
        text += strspn(text, " \t");
        if(strcasecmp(text, "keep-alive") == 0){
            m_linger = true;
        }
    }else if(strncasecmp(text, "Content-Length:", 15) == 0){
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    }else if(strncasecmp(text, "Host:", 5) == 0){
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }else{
        printf("oop !unknow header %s\n", text);
    }
    return NO_REQUEST;  /* continue to parse */
}

/* 没有真正解析HTTP请求的消息体，只是判断它是否被完整的读入了 */
http_conn::HTTP_CODE http_conn::parse_content(char* text){
    if(m_read_idx >= (m_content_length + m_checked_idx)){
        text[m_content_length] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

/* 
    当得到一个完整、正确的HTTP请求时，我们就分析目标文件的属性，
    如果目标文件存在、对所有用户可读，且不是目录，则使用mmap将其
    映射到内存地址m_file_address处，并告诉调用者获取文件成功
*/
http_conn::HTTP_CODE http_conn::do_request(){
    
    strcpy(m_real_file, doc_root);  /* char *strcpy(char *dest, const char *src); */
    int len = strlen(doc_root);
    strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);  /* char *strncpy(char *dest, const char *src, size_t n); */

    /* 
        获取m_real_file文件的相关的状态信息，-1失败，0成功 
        stat: get file stat
    */
    if(stat(m_real_file, &m_file_stat) < 0){
        return NO_REQUEST;
    }
    if(!(m_file_stat.st_mode & S_IROTH)){   /* Determine read and write permissions */
        return FORBIDDEN_REQUEST;
    }
    if(S_ISDIR(m_file_stat.st_mode)){   /* detect whether dir */
        return BAD_REQUEST;
    }

    int fd = open(m_real_file, O_RDONLY);   /* read only open file */

    /* 
        create Create a memory map ;
        void *mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);
        mmap: map files or devices into memory
    */
    m_file_address = (char*)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(m_file_address == (void*)-1){
        perror("mmap:");
        return BAD_REQUEST;
    }
    close(fd);
    return FILE_REQUEST;
}



/*
    public memeber function
*/

/* deal client request */
void http_conn::process(){
    /* Parse http request */
    HTTP_CODE read_ret = process_read();
    if(read_ret == NO_REQUEST){
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }

    /* Generate response */
    printf("generate response\n");
}

void http_conn::init(int sockfd, const sockaddr_in &caddr){
    m_sockfd = sockfd;
    m_address = caddr;
    
    int reuse = 1;  /* set port reuse */
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    addfd(m_epollfd, m_sockfd, true);
    m_user_count++;

    init();
}

void http_conn::close_conn(){
    if(m_sockfd != -1){
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;     /* 关闭一个连接，将客户总数-1 */
    }
}

bool http_conn::read(){
    printf("read---------\n");
    if(m_read_idx >= READ_BUFFER_SIZE){
        return false;
    }
    int bytes_read = 0;
    while(true){
        /* ET触发，循环读完 */
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if(bytes_read == -1){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                break;  /* 没有数据 */
            }
            return false;
        }else if(bytes_read == 0){
            printf("------close-------\n");
            return false;   /* client close */
        }
        m_read_idx += bytes_read;
        printf("read data: %s\n", m_read_buf);
    }
    return true;
}

bool http_conn::write(){
    printf("write\n");
    return true;
}





