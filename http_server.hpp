#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sys/socket.h>
#include <queue>
#include <map>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include "sql.hpp"

using namespace std;

class HTTPServer
{
private:
    const int BUF_SIZE = 1024;
    map<string, string> REQUEEST_LINE;
    map<string, string> REQUEEST_HEADER;
    string REQUEEST_CONTENT;

    map<string, string> RESPONSE_LINE;
    map<string, string> RESPONSE_HEADER;
    string RESPONSE_CONTENT;

    bool m_finished;
    int m_fd;

public:
    HTTPServer(int fd);
    ~HTTPServer();
    void recv_message();
    void send_message();
    bool is_finished();

private:
    string m_leave;
    void m_message_parse(string message);
    map<string, string> m_line_parse(string line);
    map<string, string> m_kv_content();

private:
    string username;
    string password;
    string m_GMT_time();
    void m_GET();
    void m_POST();
    void m_PUT();
    void m_DELETE();
    void m_HEAD();
    void m_OPTIONS();
    void m_TRACE();
    void m_BadRequest();

    // deal tasks by REQUEEST
};