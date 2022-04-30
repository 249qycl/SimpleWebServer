#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <queue>
#include <map>
#include <mutex>
#include <cstring>
#include <thread>
#include <algorithm>
#include "http_server.hpp"
#include "thread_pool.hpp"
#include "logger.hpp"
using namespace std;

struct FDInfo
{
    uint32_t event;
    std::chrono::_V2::system_clock::time_point lastActive; // clock()500*1000[500ms]
    HTTPServer* entity;
};

class Reactor
{
private:
    int m_epfd;
    int m_server_fd;
    bool m_reactor;
    bool m_server;
    int m_numFD;
    int m_epollsize;

    map<int, FDInfo> m_fds; //未枷锁
    vector<int> m_timeoutFD;
    queue<int> m_qFD;

    thread *m_acceptor;
    thread *m_selector;
    thread *m_dispatcher;
    thread *m_handler;
    thread *m_timeout;

    mutex m_addLock;
    mutex m_modLock;
    mutex m_delLock;
    queue<int> m_addFD;
    queue<int> m_modFD;
    queue<int> m_delFD;

    mutex m_taskLock;
    queue<function<void()>> m_tasks;

public:
    Reactor();
    ~Reactor();
    void init_server(int port, int listenNum, int epoll_size);
    void selector();
    void acceptor();
    void dispatcher();
    void handler();
    void close_reactor();

private:
    void m_writer(int fd);
    void m_reader(int fd);
    void m_del_timeout();
};
