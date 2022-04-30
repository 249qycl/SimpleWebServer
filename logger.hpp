#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <mutex>
#include <queue>
#include <thread>
#include <string>
#include <functional>
#include <chrono>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <iomanip>

using namespace std;
class Logger
{
private:
    const char *m_dir;
    int m_maxLogs;
    int m_logNums;
    ofstream logfile;
    mutex m_write;
    mutex m_queue;
    queue<string> infoQueue;
    thread *m_thread;

private:
    Logger();
    ~Logger();

public:
    static bool m_log;
    void init(const char *dir, int maxLogs);
    static Logger *get_instance();
    template <typename... Args>
    void add_log(int level, const char *fmt, Args... args);
    void async_write();
    void close();
};

template <typename... Args>
void Logger::add_log(int level, const char *fmt, Args... args)
{
    char buff[100] = {0};
    string info;
    m_queue.lock();
    stringstream time_str;
    time_t t = chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time_str << std::put_time(std::localtime(&t), "%Y-%m-%d %X");
    info.append(time_str.str());
    switch (level)
    {
    case 0:
        info.append("[debug]\t");
        break;
    case 2:
        info.append("[warn]\t");
        break;
    case 3:
        info.append("[erro]\t");
        break;
    default:
        info.append("[info]\t");
        break;
    }
    sprintf(buff, fmt, args...);
    info.append(buff);
    infoQueue.push(info);
    m_queue.unlock();
}
#define LOG_DEBUG(fmt, ...)                                         \
    {                                                               \
        if (Logger::m_log == true)                                  \
            Logger::get_instance()->add_log(0, fmt, ##__VA_ARGS__); \
    }
#define LOG_INFO(fmt, ...)                                          \
    {                                                               \
        if (Logger::m_log == true)                                  \
            Logger::get_instance()->add_log(1, fmt, ##__VA_ARGS__); \
    }
#define LOG_WARN(fmt, ...)                                          \
    {                                                               \
        if (Logger::m_log == true)                                  \
            Logger::get_instance()->add_log(2, fmt, ##__VA_ARGS__); \
    }
#define LOG_ERRO(fmt, ...)                                          \
    {                                                               \
        if (Logger::m_log == true)                                  \
            Logger::get_instance()->add_log(3, fmt, ##__VA_ARGS__); \
    }
