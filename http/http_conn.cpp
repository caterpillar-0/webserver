#include "http_conn.h"

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
void movfd(int epollfd, int fd, int ev){
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
void init(){
    
   
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
        printf("got one line: %s\n", m_start_line);
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


http_conn::HTTP_CODE http_conn::parse_request_line(char* text){
    return NO_REQUEST;

}
http_conn::HTTP_CODE http_conn::parse_headers(char* text){
    return NO_REQUEST;

}
http_conn::HTTP_CODE http_conn::parse_content(char* text){
    return NO_REQUEST;

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





