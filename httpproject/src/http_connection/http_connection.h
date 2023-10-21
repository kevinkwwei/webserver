//
// Created by Kevin Wei on 2023/9/29.
//

#pragma once

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <map>
#include <string>
#include "mysql/sql_connection_pool.h"
#include "http_structs.h"

class http_connection {
public:
    http_connection() {}

    ~http_connection() {}

    /**
     * 初始化连接
     * @param sockfd 连接文件描述符
     * @param addr client_address
     * @param user 用户信息
     * @param passwd 用户密码
     * @param sqlname 数据库名字
     */
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, std::string user, std::string passwd,
              std::string sqlname);

    /**
     * 关闭连接
     * @param real_close
     */
    void close_conn(bool real_close = true);


    /**
     * 处理请求解析
     */
    void process();

    /**
     * 读操作
     * @return
     */
    bool read_once();

    /**
     * 写操作
     * @return
     */
    bool write();

    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state;  //读为0, 写为1

    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(sql_connection_pool *connPool);
    int timer_flag;
    int improv;

private:
    void init();
    // 报文解析
    HTTP_CODE process_read();
    // 报文相应
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() {
        // 返回现在读到的位置
        return m_read_buf + m_start_line;
    };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];
    long m_read_idx;
    long m_checked_idx;
    int m_start_line;
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    CHECK_STATE m_check_state;
    METHOD m_method;
    char m_real_file[FILENAME_LEN];
    char *m_url;
    char *m_version;
    char *m_host;
    long m_content_length;
    bool m_linger;
    char *m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send;
    int bytes_have_send;
    char *doc_root;

    std::map<std::string, std::string> m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];

};

