
#include "sql.hpp"

void SQLPool::init(const char *host, const char *user, const char *pass, const char *db, int poolSize)
{
    m_host = host;
    m_user = user;
    m_pass = pass;
    m_db = db;
    m_poolSize = poolSize;

    MYSQL *connection = new MYSQL;
    mysql_init(connection);
    if (nullptr == mysql_real_connect(connection, m_host, m_user, m_pass, m_db, 0, NULL, CLIENT_FOUND_ROWS))
        LOG_ERRO("mysql_real_connect failed");
    m_mutexSql.lock();
    m_connQueue.push(connection);
    m_mutexSql.unlock();

    m_thread = new thread(mem_fn(&SQLPool::m_adaptive_pool), this);
}
SQLPool::SQLPool()
{
    m_pool = true;
    m_currConn = 0;
    m_minRatio = 0.2;
    m_maxRatio = 0.8;
}

SQLPool::~SQLPool()
{
    while (m_connQueue.empty() == false)
    {
        m_mutexSql.lock();
        MYSQL *conn = m_connQueue.front();
        mysql_close(conn);
        delete conn;
        m_connQueue.pop();
        m_mutexSql.unlock();
    }
    m_thread->join();
    delete m_thread;
    LOG_INFO("SQLPool close successfully");
}
SQLPool *SQLPool::get_instance()
{
    static SQLPool instance;
    return &instance;
}
void SQLPool::close()
{
    m_pool = false;
}
MYSQL *SQLPool::get_connection()
{
    MYSQL *conn = nullptr;
    while (1)
    {
        m_mutexSql.lock();
        if (m_connQueue.empty())
        {
            m_mutexSql.unlock();
            continue;
        }
        else
        {
            conn = m_connQueue.front();
            m_connQueue.pop();
            m_currConn++;
            m_mutexSql.unlock();
            break;
        }
    }
    return conn;
}

void SQLPool::release_connection(MYSQL *conn)
{
    m_mutexSql.lock();
    m_connQueue.push(conn);
    m_currConn--;
    m_mutexSql.unlock();
}
void SQLPool::m_adaptive_pool()
{
    while (m_pool)
    {
        if (m_currConn / (m_connQueue.size() + 0.01) < m_minRatio && m_connQueue.size() > 1)
        { //缩容
            m_mutexSql.lock();
            MYSQL *conn = m_connQueue.front();
            m_connQueue.pop();
            m_mutexSql.unlock();
            mysql_close(conn);
            delete conn;
        }
        else if (m_currConn / (m_connQueue.size() + 0.01) > m_maxRatio && m_connQueue.size() < m_poolSize)
        {
            //扩容
            MYSQL *connection = new MYSQL;
            mysql_init(connection);
            if (nullptr == mysql_real_connect(connection, m_host, m_user, m_pass, m_db, 0, NULL, CLIENT_FOUND_ROWS))
                LOG_ERRO("mysql_real_connect failed");
            m_mutexSql.lock();
            m_connQueue.push(connection);
            m_mutexSql.unlock();
        }

        auto fu = chrono::system_clock::now() + chrono::milliseconds(500);
        while (chrono::system_clock::now() < fu)
        {
            this_thread::yield();
        }
    }
}

// field ('value','value')
bool SQLPool::create_data(MYSQL *conn, const char *table, vector<string> fields, vector<string> values)
{
    string info;
    info.append("insert into ");
    info.append(table);
    info.append("(");
    for (int i = 0; i < fields.size(); i++)
    {
        info.append(fields[i]);
        if (i != fields.size() - 1)
            info.append(",");
    }
    info.append(") values");
    for (int i = 0; i < values.size(); i++)
    {
        info.append(values[i]);
        if (i != values.size() - 1)
            info.append(",");
    }
    if (mysql_query(conn, info.c_str()) == 0)
        return true;
    else
        return false;
}
void SQLPool::delete_data(MYSQL *conn, const char *table)
{
}

bool SQLPool::find_data(MYSQL *conn, const char *table, const char *field, const char *value)
{
    string info;
    info.append("select ");
    info.append("*");
    info.append(" from ");
    info.append(table);
    info.append(" where ");
    info.append(field);
    info.append("='");
    info.append(value);
    info.append("'");

    if (mysql_query(conn, info.c_str()) != 0)
        LOG_ERRO("sql.cpp 145:mysql_query failed");
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == 0)
        return false;
    else if (mysql_num_rows(result) > 0)
        return true;
    else
        return false;
}
void SQLPool::update_data(MYSQL *conn, const char *table)
{
}

int main_sql1()
{
    MYSQL connection3;

    mysql_init(&connection3);

    mysql_real_connect(&connection3, "localhost", "root", "sql", "webdata", 0, NULL, CLIENT_FOUND_ROWS);
    mysql_query(&connection3, "select * from  userInfo limit 100");

    MYSQL_RES *result = mysql_store_result(&connection3);
    //遍历字段
    for (int i = 0; i < mysql_num_fields(result); i++)
    {
        MYSQL_FIELD *field = mysql_fetch_field_direct(result, i);
        cout << field->name << endl;
    }
    //获取下一行
    auto row = mysql_fetch_row(result);
    while (row != NULL)
    {
        //逐列读取
        for (int i = 0; i < mysql_num_fields(result); i++)
        {
            cout << row[i] << "\t";
        }
        cout << endl;
        row = mysql_fetch_row(result);
    }
    queue<MYSQL *> q;
    q.push(&connection3);
    MYSQL *conn = q.front();
    mysql_close(conn);
    cout << "init error" << endl;
    return 0;
}
