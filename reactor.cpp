#include "reactor.hpp"

Reactor::Reactor()
{
    m_numFD = 0;
    m_epollsize = 0;
    m_reactor = true;
    m_server = true;
}
Reactor::~Reactor()
{
    m_acceptor->join();
    m_selector->join();
    m_dispatcher->join();
    m_handler->join();
    m_timeout->join();
    delete m_acceptor;
    delete m_selector;
    delete m_dispatcher;
    delete m_handler;
    delete m_timeout;
    close(m_server_fd);
}
void Reactor::init_server(int port, int listenNum, int epoll_size)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int)) != 0)
        LOG_ERRO("couldn't set SO_REUSEADDR");

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
        LOG_ERRO("Failed to bind to server")

    if (listen(server_fd, listenNum) < 0)
        LOG_ERRO("Failed to listen on server")
    m_server_fd = server_fd;
    m_epfd = epoll_create(epoll_size);
    m_epollsize = epoll_size;
    m_acceptor = new thread(mem_fn(&Reactor::acceptor), this);
    m_selector = new thread(mem_fn(&Reactor::selector), this);
    m_dispatcher = new thread(mem_fn(&Reactor::dispatcher), this);
    m_handler = new thread(mem_fn(&Reactor::handler), this);
    m_timeout = new thread(mem_fn(&Reactor::m_del_timeout), this);
}
void Reactor::acceptor()
{
    while (m_server)
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        while (m_numFD >= m_epollsize)
            ;
        int fd = accept(m_server_fd, (sockaddr *)&client_addr, &client_len);
        m_addLock.lock();
        m_addFD.push(fd);
        m_addLock.unlock();
    }
}
//注册
void Reactor::selector()
{
    while (m_reactor)
    {
        //添加：
        while (!m_addFD.empty())
        {
            m_addLock.lock();
            int fd = m_addFD.front();

            m_addFD.pop();
            m_addLock.unlock();

            if (m_fds.find(fd) != m_fds.end()) //前后两个连接撞车
                cout << "fd is existing" << endl;

            epoll_event event;
            event.events = EPOLLIN | EPOLLONESHOT;
            event.data.fd = fd;
            epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &event);

            m_fds[fd].event = event.events;
            m_fds[fd].lastActive = chrono::system_clock::now();
            m_fds[fd].entity = new HTTPServer(fd);
            
            if (m_fds[fd].entity==nullptr)
            { // cout << "unfinished:" << fd << endl;
                time_t t = chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                cout << "create:" << fd <<"\t"<< std::put_time(std::localtime(&t), "%Y-%m-%d %X") << endl;
            }

            m_numFD++;
        }
        //修改：
        while (!m_modFD.empty())
        {
            m_modLock.lock();
            int fd = m_modFD.front();
            m_modFD.pop();
            m_modLock.unlock();

            epoll_event event;
            if (m_fds[fd].event & EPOLLOUT)
                event.events = EPOLLIN | EPOLLONESHOT;
            else
                event.events = EPOLLOUT | EPOLLONESHOT;
            event.data.fd = fd;
            if (epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &event) < 0)
                LOG_ERRO("epoll_ctl_mod failed");
            m_fds[fd].event = event.events;
            m_fds[fd].lastActive = chrono::system_clock::now();
        }
        //删除 [加进了删除队列，却未真正完成消除，wait成功二次触发]
        while (!m_delFD.empty())
        {
            m_delLock.lock();
            int fd = m_delFD.front();
            m_delFD.pop();
            m_delLock.unlock();
            //解决finished与timeout同时作用于一个fd的冲突
            if (m_fds.find(fd) == m_fds.end())
                continue;
            epoll_event event;
            if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, &event))
                LOG_ERRO("epoll_ctl_del failed");
            if (m_numFD > 0)
                m_numFD--;

            if (!m_fds[fd].entity->is_finished())
            { // cout << "unfinished:" << fd << endl;
                time_t t = chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                cout << "unfinished:" << fd <<"\t"<< std::put_time(std::localtime(&t), "%Y-%m-%d %X") << endl;
            }

            delete m_fds[fd].entity;
            m_fds.erase(fd);
            close(fd);
        }
    }
}
//分离器
void Reactor::dispatcher()
{
    while (m_reactor)
    {
        int maxevents = m_numFD;
        epoll_event *events = new epoll_event[maxevents];
        memset(events, 0, sizeof(epoll_event) * maxevents);
        int event_nums = epoll_wait(m_epfd, events, maxevents, 0);
        if (event_nums < 0)
        {
            delete[] events;
            continue;
        }
        for (int i = 0; i < event_nums; i++)
        {
            if (events[i].events & EPOLLOUT)
            { //可写
                int fd = events[i].data.fd;
                function<void(int)> write = [this](int fd)
                {
                    this->m_writer(fd);
                };
                function<void()> task = bind(write, fd);
                m_taskLock.lock();
                m_tasks.push(task);
                m_taskLock.unlock();
            }
            else if (events[i].events & EPOLLIN)
            { //可读
                int fd = events[i].data.fd;
                function<void(int)> read = [this](int fd)
                {
                    this->m_reader(fd);
                };
                function<void()> task = bind(read, fd);
                m_taskLock.lock();
                m_tasks.push(task);
                m_taskLock.unlock();
            }
            else
                LOG_WARN("Event not in EPOLLIN or EPOLLOUT");
        }
        delete[] events;
    }
}

//调度线程池处理任务
void Reactor::handler()
{
    while (m_reactor)
    {
        m_taskLock.lock();
        while (m_tasks.empty() == false)
        {
            ThreadPool::get_instance()->push_task(m_tasks.front());
            m_tasks.pop();
        }
        m_taskLock.unlock();
    }
}

void Reactor::close_reactor()
{
    m_server = false;
}

void Reactor::m_reader(int fd)
{

    if (m_fds[fd].entity == nullptr) //对象意外消失，导致超时
    {
        time_t t = chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        cout << "entity not found:" << fd <<"\t"<< std::put_time(std::localtime(&t), "%Y-%m-%d %X") << endl;
        // cout << "entity not found:" << fd << endl;
        return;
    }

    m_fds[fd].entity->recv_message();
    if (m_fds[fd].entity->is_finished())
    {
        m_delLock.lock();
        m_delFD.push(fd);
        m_delLock.unlock();
    }
    else
    {
        m_modLock.lock();
        m_modFD.push(fd);
        m_modLock.unlock();
    }
}
void Reactor::m_writer(int fd)
{

    m_fds[fd].entity->send_message();
    if (m_fds[fd].entity->is_finished())
    {
        m_delLock.lock();
        m_delFD.push(fd);
        m_delLock.unlock();
    }
    else
    {
        m_modLock.lock();
        m_modFD.push(fd);
        m_modLock.unlock();
    }
}

// 5秒检测一次
void Reactor::m_del_timeout()
{
    while (m_reactor)
    {
        auto fu = chrono::system_clock::now() + chrono::seconds(5);
        while (chrono::system_clock::now() < fu)
        {
            this_thread::yield();
        }
        auto now = chrono::system_clock::now();
        m_delLock.lock();
        for (auto it = m_fds.begin(); it != m_fds.end(); ++it)
        {
            if (now - it->second.lastActive > chrono::seconds(5))
            {
                m_delFD.push(it->first);
                time_t t = chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                cout << "timeout:" << it->first <<"\t"<< std::put_time(std::localtime(&t), "%Y-%m-%d %X") << endl;
            }
        }
        m_delLock.unlock();
        //保证任务全部处理完毕
        if (m_server == false && m_fds.size() == 0 && m_addFD.size() == 0 && m_modFD.size() == 0 && m_delFD.size() == 0)
            m_reactor = false;
    }
}
//关服务端，拒绝新连接，处理已有连接