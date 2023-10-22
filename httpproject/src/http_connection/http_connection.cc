//
// Created by Kevin Wei on 2023/9/29.
//

#include "http_connection.h"
#include "log/log.h"

//定义http响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

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

//读取浏览器端发来的全部数据
bool http_connection::read_once() {
    if (m_read_idx >= READ_BUFFER_SIZE)
    {
        return false;
    }
    int bytes_read = 0;

    //LT读取数据
    if (0 == m_TRIGMode)
    {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        m_read_idx += bytes_read;

        if (bytes_read <= 0)
        {
            return false;
        }

        return true;
    }
        //ET读数据
    else
    {
        // et下一次性读完
        while (true)
        {
            bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
            if (bytes_read == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if (bytes_read == 0)
            {
                return false;
            }
            m_read_idx += bytes_read;
        }
        return true;
    }
}

// 像前端发送/写数据
bool http_connection::write() {
    int temp = 0;
    // 要发的数据为空
    if (bytes_to_send == 0) {
        // 不写了，变成读，然后都重置
        modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
        init();
        return true;
    }
    while (1) {
        // 写数据
        temp = writev(m_sockfd, m_iv, m_iv_count);
        // 发送失败
        if (temp < 0) {
            // 如果是缓冲区满了
            if (errno == EAGAIN) {
                // 等待下次继续
                modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
                return true;
            }
            unmap();
            return false;
        }
        // 发送成功了
        bytes_have_send += temp;
        bytes_to_send -= temp;
        // 发完了第一部分
        if (bytes_have_send >= m_iv[0].iov_len) {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        } else {
            // 第一部分都没发送完，全部重发
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }
        // 无数据可发了
        if (bytes_to_send <= 0) {
            unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
            if (m_linger) {
                init();
                return true;
            } else {
                return false;
            }
        }
    }
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

void http_connection::unmap() {
    if (m_file_address) {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}

// 处理响应报文逻辑
bool http_connection::process_write(HTTP_CODE ret) {
    switch (ret) {
        case HTTP_CODE::BAD_REQUEST:
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if (!add_content(error_404_form))
                return false;
            break;
        case HTTP_CODE::INTERNAL_ERROR:
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if (!add_content(error_500_form))
                return false;
            break;
        case HTTP_CODE::FILE_REQUEST:
            add_status_line(200, ok_200_title);
            if (m_file_stat.st_size != 0) {
                add_headers(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                bytes_to_send = m_write_idx + m_file_stat.st_size;
                return true;
            } else {
                const char *ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if (!add_content(ok_string))
                    return false;
            }
            break;
        case HTTP_CODE::FORBIDDEN_REQUEST:
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if (!add_content(error_403_form))
                return false;
            break;
        default:
            return false;
    }
    // 为了之后writev使用
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}

// 对于所有add，本质都调用add_response
bool http_connection::add_response(const char *format, ...) {
    // 判断现在的写入位置是否还能继续写
    if (m_write_idx >= WRITE_BUFFER_SIZE)
        return false;
    // 保存 ... 的值
    va_list arg_list;
    // 初始化arg_list
    va_start(arg_list, format);
    auto curr_write_buff = m_write_buf + m_write_idx;
    auto curr_write_buff_size = WRITE_BUFFER_SIZE - 1 - m_write_idx;
    // 将arg_list里的可变参数写入curr_write_buff， len大于0且小于curr_write_buff_size证明完全写入
    int len = vsnprintf(curr_write_buff, curr_write_buff_size, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx)) {
        // 清空
        va_end(arg_list);
        return false;
    }
    // 更新位置
    m_write_idx += len;
    va_end(arg_list);

    LOG_INFO("request:%s", m_write_buf);

    return true;
}

// 处理request，产出response
HTTP_CODE http_connection::do_request() {

}

bool http_connection::add_headers(int content_length) {
    return add_content_length(content_length) && add_linger() &&
           add_blank_line();
}

bool http_connection::add_blank_line() {
    return add_response("%s", "\r\n");
}

bool http_connection::add_content_type() {
    return add_response("Content-Type:%s\r\n", "text/html");
}

bool http_connection::add_content_length(int content_length) {
    return add_response("Content-Length:%d\r\n", content_length);
}

bool http_connection::add_linger() {
    return add_response("Connection:%s\r\n", (m_linger == true) ? "keep-alive" : "close");
}

bool http_connection::add_content(const char *content) {
    return add_response("%s", content);
}

bool http_connection::add_status_line(int status, const char *title) {
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}
