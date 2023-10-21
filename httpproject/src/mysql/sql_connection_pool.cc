//
// Created by Kevin Wei on 2023/10/1.
//

#include "sql_connection_pool.h"
#include "log/log.h"

void
sql_connection_pool::init(std::string url, std::string user, std::string password, std::string database_name, int port,
                          int max_conn, int close_log) {
    m_url = url;
    m_User = user;
    m_PassWord = password;
    m_DatabaseName = database_name;
    m_Port = port;
    m_close_log = close_log;
    // init sem变量
    reserve = Semaphore(m_FreeConn);

    m_MaxConn = max_conn;

    for (int i = 0; i < max_conn; i++) {
        MYSQL *con = NULL;
        con = mysql_init(con);
        if (con == NULL) {
            LOG_ERROR("MYSQL ERROR");
            exit(1);
        }
        con = mysql_real_connect(con, url.c_str(), user.c_str(), password.c_str(), database_name.c_str(), port, NULL,
                                 0);
        if (con == NULL) {
            LOG_ERROR("MYSQL ERROR");
            exit(1);
        }
        connList.push_back(con);
        m_FreeConn++;
    }
}

sql_connection_pool *sql_connection_pool::GetInstance() {
    static sql_connection_pool conn_pool;
    return &conn_pool;
}

void sql_connection_pool::DestoryPool() {
    if (connList.size() == 0) {
        return;
    }
    lock.Lock();
    for (auto conn: connList) {
        mysql_close(conn);
    }
    m_CurConn = 0;
    m_FreeConn = 0;
    connList.clear();
    lock.UnLock();
}

MYSQL *sql_connection_pool::GetConnection() {
    if (m_FreeConn == 0 || connList.size()) {
        return NULL;
    }
    MYSQL *conn = NULL;
    reserve.Wait();
    lock.Lock();
    conn = connList.front();
    connList.pop_front();
    m_FreeConn--;
    m_CurConn++;
    lock.UnLock();
    return conn;
}

int sql_connection_pool::GetFreeConn() {
    return m_FreeConn;
}

bool sql_connection_pool::ReleaseConnection(int *conn) {
    if (conn == NULL) {
        return false;
    }
    lock.Lock();
    connList.push_back(conn);
    m_FreeConn++;
    m_CurConn--;
    lock.UnLock();
    reserve.Post();
    return true;
}

sql_connection_pool::sql_connection_pool() {
    m_CurConn = 0;
    m_FreeConn = 0;
}

connectionRAII::connectionRAII(MYSQL **con, sql_connection_pool *connPool) {
    auto conn = connPool->GetConnection();
    conRAII = conn;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII() {
    poolRAII->ReleaseConnection(conRAII);
}
