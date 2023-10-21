//
// Created by Kevin Wei on 2023/9/29.
//

#include "http_connection.h"
#include "log/log.h"

const std::string http_200_title = "OK";
const std::string bad_400_title = "Bad Request";
const std::string error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const std::string error_403_title = "Forbidden";
const std::string error_403_form = "You do not have permission to get file form this server.\n";
const std::string error_404_title = "Not Found";
const std::string error_404_form = "The requested file was not found on this server.\n";
const std::string error_500_title = "Internal Error";
const std::string error_500_form = "There was an unusual problem serving the request file.\n";

//对文件描述符设置非阻塞
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}


//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode) {
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//从内核时间表删除描述符
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//将事件重置为EPOLLONESHOT
void modfd(int epollfd, int fd, int ev, int TRIGMode) {
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_connection::m_user_count = 0;
int http_connection::m_epollfd = -1;

void http_connection::init(int sockfd, const sockaddr_in &addr, char *root, int TRIGMode,
                           int close_log, std::string user, std::string passwd, std::string sqlname) {
    m_sockfd = sockfd;
    m_address = addr;
    doc_root = root;
    m_TRIGMode = m_TRIGMode;
    m_close_log = close_log;

    // 初始化epollfd
    addfd(m_epollfd, sockfd, true, m_TRIGMode);
    m_user_count++;

    // 初始化数据库配置
    strcpy(sql_user, user.c_str());
    strcpy(sql_passwd, passwd.c_str());
    strcpy(sql_name, sqlname.c_str());

    // 初始化其他变量
    init();

}

void http_connection::init() {
    mysql = NULL;
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    cgi = 0;
    m_state = 0;
    timer_flag = 0;
    improv = 0;

    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

void http_connection::close_conn(bool real_close) {
    if (real_close && (m_sockfd != -1)) {
        std::cout << "close sockfd : " << m_sockfd << std::endl;
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

void http_connection::process() {
    // 报文解析
    auto read_result = process_read();
    // 请求不完整， 继续接受请求
    if (read_result == NO_REQUEST) {
        //epoll 读
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }

    // 请求完整， 相应报文
    auto write_result = process_write(read_result);
    if (!write_result) {
        close_conn();
    }
    // epoll 写
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}


bool http_connection::read_once() {
    return false;
}

bool http_connection::write() {
    return false;
}

// 处理接受请求报文
HTTP_CODE http_connection::process_read() {
    // 初始化一些状态
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;
    // 进入循环处理报文
    while ((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) ||
           ((line_status = parse_line()) == LINE_OK)) {
        // 返回现在读到的位置
        text = get_line();
        // 更新下一次的开始位置，这里可以优化
        //m_start_line是每一个数据行在m_read_buf中的起始位置
        //m_checked_idx表示从状态机在m_read_buf中读取的位置
        m_start_line = m_checked_idx;
        LOG_INFO("%s", text);

        // 状态机
        switch (m_check_state) {
            case CHECK_STATE_REQUESTLINE:
                ret = parse_request_line(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                } else if (ret == GET_REQUEST) {
                    return do_request();
                }
                break;
            case CHECK_STATE_HEADER:
                ret = parse_headers(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                } else if (ret == GET_REQUEST) {
                    return do_request();
                }
                break;
            case CHECK_STATE_CONTENT:
                ret = parse_content(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                } else if (ret == GET_REQUEST) {
                    return do_request();
                }
                break;
            default:
                return INTERNAL_ERROR;
        }
        return NO_REQUEST;
    }
}

//解析http请求行，获得请求方法，目标url及http版本号
HTTP_CODE http_connection::parse_request_line(char *text) {
    // 第一个空格的位置，指针
    // GET /562f25980001b1b106000338.jpg HTTP/1.1
    m_url = strpbrk(text, " \t");
    //如果没有空格或\t，则报文格式有误
    if (!m_url) {
        return BAD_REQUEST;
    }
    // \t位置改成 \0, 从中截断text，只取方法
    *m_url = '\0';
    // m_url是后面的开头
    *m_url++;
    char *method = text;
    // 不区分大小写的比较
    if (strcasecmp(method, "GET") == 0)
        m_method = GET;
    else if (strcasecmp(method, "POST") == 0) {
        m_method = POST;
        cgi = 1;
    } else {
        return BAD_REQUEST;
    }
    //移到第一个有内容的字符处
    m_url += strspn(m_url, " \t");

    //同上，获得version号
    m_version = strpbrk(m_url, " \t");
    if (!m_version) {
        return BAD_REQUEST;
    }
    *m_version = '\0';
    *m_version++;

    m_version += strspn(m_version, " \t");
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
        return BAD_REQUEST;

    // 比较前7位
    if (strncasecmp(m_url, "http://", 7) == 0) {
        m_url += 7;
        // 比如 http://google.com
        // 返回 /google.com
        m_url = strchr(m_url, '/');
    }

    // 比较前8位
    if (strncasecmp(m_url, "https://", 8) == 0) {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }
    if (!m_url || m_url[0] != '/')
        return BAD_REQUEST;
    //当url为/时，显示判断界面
    if (strlen(m_url) == 1)
        strcat(m_url, "judge.html");
    m_check_state = CHECK_STATE_HEADER;
    // 只是获取url version 等信息
    return NO_REQUEST;
}

HTTP_CODE http_connection::parse_headers(char *text) {
    if (text[0] == '\0') {
        if (m_content_length != 0) {
            //状态不变则一直居于这里
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        // 之后有内容的地方
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            m_linger = true;
        }
    } else if (strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        // str to int
        m_content_length = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    } else {
        LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

HTTP_CODE http_connection::parse_content(char *text) {
    // 本项目中只对post使用
    if (m_read_idx >= (m_content_length + m_checked_idx)) {
        text[m_content_length] = '\0';
        //POST请求中最后为输入的用户名和密码
        m_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

// 处理request，产出response
HTTP_CODE http_connection::do_request() {

}
