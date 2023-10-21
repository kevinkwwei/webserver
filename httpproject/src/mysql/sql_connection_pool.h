//
// Created by Kevin Wei on 2023/10/1.
//
#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "currency/mutex_locker.h"
#include "currency/semaphore.h"

class sql_connection_pool {
public:
    MYSQL *GetConnection();

    bool ReleaseConnection(MYSQL *conn);

    int GetFreeConn();

    void DestoryPool();

    static sql_connection_pool *GetInstance();

    void
    init(std::string url, std::string user, std::string password, std::string database_name, int port, int max_conn,
         int close_log);

    std::string m_url;			 //主机地址
    std::string m_Port;		 //数据库端口号
    std::string m_User;		 //登陆数据库用户名
    std::string m_PassWord;	 //登陆数据库密码
    std::string m_DatabaseName; //使用数据库名
    int m_close_log;	//日志开关

private:
    sql_connection_pool();
    ~sql_connection_pool();

    int m_MaxConn;  //最大连接数
    int m_CurConn;  //当前已使用的连接数
    int m_FreeConn; //当前空闲的连接数
    MutexLocker lock;
    List<MYSQL *> connList;
    Semaphore reserve;
};

class connectionRAII{

public:
    connectionRAII(MYSQL **con, sql_connection_pool *connPool);
    ~connectionRAII();

private:
    MYSQL *conRAII;
    sql_connection_pool *poolRAII;
};