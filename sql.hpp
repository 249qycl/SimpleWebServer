#pragma once
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <mysql/mysql.h>
#include <iostream>
#include <sstream>
#include "logger.hpp"
using namespace std;

class SQLPool
{
private:
    const char *m_host;
    const char *m_user;
    const char *m_pass;
    const char *m_db;
    float m_minRatio;
    float m_maxRatio;
    int m_poolSize;
    bool m_pool;
    int m_currConn;
    thread *m_thread;
    queue<string> m_queryQueue;
    mutex m_mutexSql;
    queue<MYSQL *> m_connQueue;

public:
    void init(const char *host, const char *user, const char *pass, const char *db, int poolSize);
    static SQLPool *get_instance();
    MYSQL *get_connection();
    void release_connection(MYSQL *conn);

    bool create_data(MYSQL *conn,const char*table,vector<string> fields,vector<string> values);
    void delete_data(MYSQL *conn,const char*table);
    bool find_data(MYSQL *conn,const char*table,const char *field,const char *value);
    void update_data(MYSQL *conn,const char*table);

    void close();

private:
    SQLPool();
    ~SQLPool();
    void m_adaptive_pool();
};
