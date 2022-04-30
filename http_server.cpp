#include "http_server.hpp"

HTTPServer::HTTPServer(int fd)
{
    m_fd = fd;
    m_finished = false;
}
HTTPServer::~HTTPServer()
{
}
bool HTTPServer::is_finished()
{
    return m_finished;
}
void HTTPServer::recv_message()
{
    char *buff = new char[BUF_SIZE];
    memset(buff, 0, BUF_SIZE);
    string message(m_leave);
    if (recv(m_fd, buff, BUF_SIZE, 0) < 0)
    {
        LOG_ERRO("recv error %s", strerror(errno));
        m_finished = true;
        delete[] buff;
        return;
    }
    else
    {
        message.append(buff);
        // cout << message << endl;
        delete[] buff;
    }

    string res;
    size_t pos = message.find("Content-Length");
    if (pos != string::npos)
    {
        for (int i = pos + 15; i < message.length(); i++)
        {
            if (message[i] == ':' || message[i] == ' ')
                continue;
            else if (message[i] == '\r')
                break;
            else
                res.append(message.substr(i, 1));
        }
    }
    else if (message.empty() == false) //请求无length信息,且主体非空
    {
        m_message_parse(message);
        return;
    }
    string content = message;
    if (res.length() == 0)
    {
        m_finished = true;
        return;
    }
    size_t rnPos = message.find("\r\n\r\n");
    int len = message.length() - rnPos - 4;
    int contentLength = atoi(res.c_str());
    if (contentLength > len)
    {
        char *buff = new char[contentLength - len];
        memset(buff, 0, contentLength - len);
        if (recv(m_fd, buff, sizeof(buff), 0) < 0)
        {
            delete[] buff;
            LOG_ERRO("recv error: %s", strerror(errno));
            m_finished = true;
            return;
        }
        message.append(buff);
        delete[] buff;
        content = message;
    }
    else if (contentLength < len)
    {
        content = message.substr(0, message.length() - (len - contentLength));
        m_leave = message.substr(content.length());
    }
    m_message_parse(content);
}
void HTTPServer::m_message_parse(string message)
{
    int state = 0;
    istringstream iss(message);
    string line;

    size_t pos = message.find("\r\n\r\n");
    if (pos != string::npos)
    {
        REQUEEST_CONTENT = message.substr(pos + 4);
    }
    while (getline(iss, line, '\n'))
    {
        if (line.find("\r") != string::npos)
            line = line.replace(line.find("\r"), 1, "");
        if (state == 0)
        {
            istringstream is(line);
            string s;
            int n = 0;
            while (getline(is, s, ' '))
            {
                switch (n)
                {
                case 0:
                    REQUEEST_LINE["METHOD"] = s;
                    n++;
                    break;
                case 1:
                    REQUEEST_LINE["URI"] = s;
                    n++;
                    break;
                case 2:
                    REQUEEST_LINE["PROTOCOL"] = s;
                    n++;
                    break;
                }
            }
            state++;
        }
        else if (state == 1)
        {
            if (line.length() == 0)
                break;
            else
            {
                map<string, string> kv = m_line_parse(line);
                REQUEEST_HEADER.insert(kv.begin(), kv.end());
            }
        }
    }
}
map<string, string> HTTPServer::m_line_parse(string line)
{
    map<string, string> res;
    while (line.find("\r") != string::npos)
    {
        line.replace(line.find("\r"), 1, "");
    }
    while (line.find("\n") != string::npos)
    {
        line.replace(line.find("\n"), 1, "");
    }
    int pos = line.find(":");
    if (pos != string::npos)
        res[line.substr(0, pos)] = line.substr(pos + 1);

    return res;
}
map<string, string> HTTPServer::m_kv_content()
{
    stringstream content(REQUEEST_CONTENT);
    string part;
    map<string, string> res;
    while (getline(content, part, '&'))
    {
        stringstream kv(part);
        string str;
        string key;
        while (getline(kv, str, '='))
        {
            if (key.size() == 0)
                key = str;
            else
                res[key] = str;
        }
    }
    return res;
}

string HTTPServer::m_GMT_time()
{
    string ret;
    time_t rawTime;
    struct tm *timeInfo;
    char buff[30] = {0};
    time(&rawTime);
    timeInfo = gmtime(&rawTime);
    strftime(buff, sizeof(buff), "%a, %d %b %Y %H:%M:%S GMT", timeInfo);
    ret.append(buff);
    return ret;
}


void HTTPServer::m_GET()
{
    //若为根文件
    if (REQUEEST_LINE["URI"].compare("/") == 0)
    {
        RESPONSE_LINE["STATUS"] = "200";
        RESPONSE_LINE["TITLE"] = "OK";
        RESPONSE_HEADER["Content-Type"] = "text/html";
        RESPONSE_HEADER["Connection"] = "keep-alive";
        ifstream fin("./www/home.html");
        istreambuf_iterator<char> beg(fin), end;
        string strdata(beg, end);
        RESPONSE_CONTENT = strdata;
        fin.close();
        return;
    }
    ifstream fin("./www" + REQUEEST_LINE["URI"]);
    if (fin.good())
    {
        RESPONSE_LINE["STATUS"] = "200";
        RESPONSE_LINE["TITLE"] = "OK";
        RESPONSE_HEADER["Content-Type"] = "text/html";
        RESPONSE_HEADER["Connection"] = "close";//keep-alive
        istreambuf_iterator<char> beg(fin), end;
        string strdata(beg, end);
        RESPONSE_CONTENT = strdata;
    }
    else
    {
        RESPONSE_LINE["STATUS"] = "404";
        RESPONSE_LINE["TITLE"] = "Not Found";
        RESPONSE_HEADER["Content-Type"] = "text/html";
        RESPONSE_HEADER["Connection"] = "close";
        ifstream nfin("./www/404.html");
        if (nfin.good())
        {
            istreambuf_iterator<char> beg(nfin), end;
            string strdata(beg, end);
            RESPONSE_CONTENT = strdata;
        }
        else
            LOG_ERRO("404.html is not found");
        nfin.close();
    }
    fin.close();
}
void HTTPServer::m_POST()
{
    string message;
    if (REQUEEST_LINE["URI"].compare("/login") == 0)
    {
        map<string, string> content;
        if (REQUEEST_HEADER["Content-Type"].find("application/x-www-form-urlencoded") != string::npos)
            content = m_kv_content();
        MYSQL *conn = SQLPool::get_instance()->get_connection();
        if (SQLPool::get_instance()->find_data(conn, "userInfo", "username", content["username"].c_str()))
        {
            //回复登陆成功
            if (SQLPool::get_instance()->find_data(conn, "userInfo", "userpass", content["userpass"].c_str()))
            {
                RESPONSE_LINE["STATUS"] = "200";
                RESPONSE_LINE["TITLE"] = "OK";
                RESPONSE_HEADER["Content-Type"] = "text/html";
                RESPONSE_HEADER["Connection"] = "keep-alive";
                ifstream fin("./www/content.html");
                if (fin.good())
                {
                    istreambuf_iterator<char> beg(fin), end;
                    string strdata(beg, end);
                    RESPONSE_CONTENT = strdata;
                }
                else
                    LOG_ERRO("content.html is not found");
                fin.close();
                username = content["username"];
                password = content["userpass"];
            }
            else
            {
                RESPONSE_LINE["STATUS"] = "401";
                RESPONSE_LINE["TITLE"] = "Unauthorized";
                RESPONSE_HEADER["Content-Type"] = "text/html";
                RESPONSE_HEADER["Connection"] = "close";
                ifstream fin("./www/re-login.html");
                if (fin.good())
                {
                    istreambuf_iterator<char> beg(fin), end;
                    string strdata(beg, end);
                    RESPONSE_CONTENT = strdata;
                }
                else
                    LOG_ERRO("re-login.html is not found");
                fin.close();
            }
        }
        else
        {
            RESPONSE_LINE["STATUS"] = "401";
            RESPONSE_LINE["TITLE"] = "Unauthorized";
            RESPONSE_HEADER["Content-Type"] = "text/html";
            RESPONSE_HEADER["Connection"] = "close";
            ifstream fin("./www/register.html");
            if (fin.good())
            {
                istreambuf_iterator<char> beg(fin), end;
                string strdata(beg, end);
                RESPONSE_CONTENT = strdata;
            }
            else
                LOG_ERRO("register.html is not found");
            fin.close();
        }
        SQLPool::get_instance()->release_connection(conn);
    }
    else if (REQUEEST_LINE["URI"].compare("/register") == 0)
    {
        map<string, string> content;
        if (REQUEEST_HEADER["Content-Type"].find("application/x-www-form-urlencoded") != string::npos)
            content = m_kv_content();
        MYSQL *conn = SQLPool::get_instance()->get_connection();

        if (content["username"].empty() || content["userpass"].empty() || SQLPool::get_instance()->find_data(conn, "userInfo", "username", content["username"].c_str()))
        {
            //账户无效
            RESPONSE_LINE["STATUS"] = "200";
            RESPONSE_LINE["TITLE"] = "OK";
            RESPONSE_HEADER["Content-Type"] = "text/html";
            RESPONSE_HEADER["Connection"] = "close";
            ifstream fin("./www/re-register.html");
            if (fin.good())
            {
                istreambuf_iterator<char> beg(fin), end;
                string strdata(beg, end);
                RESPONSE_CONTENT = strdata;
            }
            else
                LOG_ERRO("re-register.html is not found");
            fin.close();
        }
        else
        {
            vector<string> fields;
            vector<string> values;
            fields.push_back("username");
            fields.push_back("userpass");
            values.push_back("('" + content["username"] + "','" + content["userpass"] + "')");
            if (SQLPool::get_instance()->create_data(conn, "userInfo", fields, values))
            {
                //回复注册成功
                RESPONSE_LINE["STATUS"] = "200";
                RESPONSE_LINE["TITLE"] = "OK";
                RESPONSE_HEADER["Content-Type"] = "text/html";
                RESPONSE_HEADER["Connection"] = "keep-alive";
                ifstream fin("./www/login.html");
                if (fin.good())
                {
                    istreambuf_iterator<char> beg(fin), end;
                    string strdata(beg, end);
                    RESPONSE_CONTENT = strdata;
                }
                else
                    LOG_ERRO("login.html is not found");
                fin.close();
            }
            else
                LOG_ERRO("faild to add fields to userInfo");
        }
        SQLPool::get_instance()->release_connection(conn);
    }
    else if (username.size() == 0)
    {
        //未授权Unauthorized
        RESPONSE_LINE["STATUS"] = "401";
        RESPONSE_LINE["TITLE"] = "Unauthorized";
        RESPONSE_HEADER["Content-Type"] = "text/html";
        RESPONSE_HEADER["Connection"] = "close";
        ifstream nfin("./www/401.html");
        if (nfin.good())
        {
            istreambuf_iterator<char> beg(nfin), end;
            string strdata(beg, end);
            RESPONSE_CONTENT = strdata;
        }
        else
            LOG_ERRO("401.html is not found");
        nfin.close();
    }
    else if (false) //无该URI
    {
        //正常业务
    }
    else
    {
        //按URI传输
    }
}
void HTTPServer::m_PUT()
{
}
void HTTPServer::m_DELETE()
{
}
void HTTPServer::m_HEAD()
{
    if (REQUEEST_LINE["URI"].compare("/") == 0)
    {
        RESPONSE_LINE["STATUS"] = "200";
        RESPONSE_LINE["TITLE"] = "OK";
        RESPONSE_HEADER["Content-Type"] = "text/html";
        RESPONSE_HEADER["Connection"] = "keep-alive";
        ifstream fin("./www/home.html");
        istreambuf_iterator<char> beg(fin), end;
        string strdata(beg, end);
        RESPONSE_HEADER["Content-Length"] = to_string(strdata.length());
        fin.close();
        return;
    }
    ifstream fin("./www" + REQUEEST_LINE["URI"]);
    if (fin.good())
    {
        RESPONSE_LINE["STATUS"] = "200";
        RESPONSE_LINE["TITLE"] = "OK";
        RESPONSE_HEADER["Content-Type"] = "text/html";
        RESPONSE_HEADER["Connection"] = "keep-alive";
        istreambuf_iterator<char> beg(fin), end;
        string strdata(beg, end);
        RESPONSE_HEADER["Content-Length"] = to_string(strdata.length());
    }
    else
    {
        RESPONSE_LINE["STATUS"] = "404";
        RESPONSE_LINE["TITLE"] = "Not Found";
        RESPONSE_HEADER["Content-Type"] = "text/html";
        RESPONSE_HEADER["Connection"] = "close";
        ifstream nfin("./www/404.html");
        if (nfin.good())
        {
            istreambuf_iterator<char> beg(nfin), end;
            string strdata(beg, end);
            RESPONSE_HEADER["Content-Length"] = to_string(strdata.length());
        }
        else
            LOG_ERRO("404.html is not found");
        nfin.close();
    }
    fin.close();
}
void HTTPServer::m_OPTIONS()
{
    RESPONSE_HEADER["Allow"] = "OPTIONS, HEAD, GET, POST, TRACE"; // trace
    RESPONSE_HEADER["Content-Length"] = "0";
    RESPONSE_HEADER["Content-Type"] = "text/html";
}
void HTTPServer::m_TRACE()
{
    RESPONSE_LINE["STATUS"] = "200";
    RESPONSE_LINE["TITLE"] = "OK";
    RESPONSE_HEADER["Content-Type"] = "message/http";
    RESPONSE_HEADER["Connection"] = "close";
    RESPONSE_CONTENT.append(REQUEEST_LINE["METHOD"] + " ");
    RESPONSE_CONTENT.append(REQUEEST_LINE["URI"] + " ");
    RESPONSE_CONTENT.append(REQUEEST_LINE["PROTOCOL"] + "\r\n");
    for (auto it = REQUEEST_HEADER.begin(); it != REQUEEST_HEADER.end(); it++)
    {
        RESPONSE_CONTENT.append(it->first);
        RESPONSE_CONTENT.append(":");
        RESPONSE_CONTENT.append(it->second);
        RESPONSE_CONTENT.append("\r\n");
    }
}
void HTTPServer::m_BadRequest()
{
    RESPONSE_LINE["STATUS"] = "400";
    RESPONSE_LINE["TITLE"] = "Bad Request";
    RESPONSE_HEADER["Content-Type"] = "text/html";
    RESPONSE_HEADER["Connection"] = "close";
    ifstream nfin("./www/400.html");
    if (nfin.good())
    {
        istreambuf_iterator<char> beg(nfin), end;
        string strdata(beg, end);
        RESPONSE_HEADER["Content-Length"] = to_string(strdata.length());
    }
    else
        LOG_ERRO("400.html is not found");
    nfin.close();
}

void HTTPServer::send_message()
{
    //填充发送报文
    if (REQUEEST_LINE["METHOD"].compare("GET") == 0)
        m_GET();
    else if (REQUEEST_LINE["METHOD"].compare("POST") == 0)
        m_POST();
    else if (REQUEEST_LINE["METHOD"].compare("PUT") == 0)
        m_PUT();
    else if (REQUEEST_LINE["METHOD"].compare("DELETE") == 0)
        m_DELETE();
    else if (REQUEEST_LINE["METHOD"].compare("HEAD") == 0)
        m_HEAD();
    else if (REQUEEST_LINE["METHOD"].compare("OPTIONS") == 0)
        m_OPTIONS();
    else if (REQUEEST_LINE["METHOD"].compare("TRACE") == 0)
        m_TRACE();
    else
        m_BadRequest();

    string message;
    message.append(REQUEEST_LINE["PROTOCOL"] + " " + RESPONSE_LINE["STATUS"] + " " + RESPONSE_LINE["TITLE"] + "\r\n");
    message.append("Server:myweb0.1 \r\n");
    message.append("Date:" + m_GMT_time() + "\r\n");
    message.append("Content-Type:" + RESPONSE_HEADER["Content-Type"] + ";charset=utf-8\r\n");

    if (RESPONSE_HEADER.find("Allow") != RESPONSE_HEADER.end())
        message.append("Allow:" + to_string(RESPONSE_HEADER["Allow"].length()) + "\r\n");
    if (RESPONSE_CONTENT.length() != 0)
        message.append("Content-Length:" + to_string(RESPONSE_CONTENT.length()) + "\r\n");
    else if (RESPONSE_HEADER.find("Content-Length") != RESPONSE_HEADER.end() && RESPONSE_HEADER["Content-Length"].length() != 0)
        message.append("Content-Length:" + RESPONSE_HEADER["Content-Length"] + "\r\n");

    message.append("\r\n");
    message.append(RESPONSE_CONTENT);
    if (RESPONSE_HEADER["Connection"].compare("close") == 0)
        m_finished = true;

    if (send(m_fd, message.c_str(), message.length(), MSG_NOSIGNAL) < 0)
    {
        m_finished = true; //客户端异常断开
        LOG_ERRO((REQUEEST_LINE["METHOD"] + ":send error").c_str());
    }
}