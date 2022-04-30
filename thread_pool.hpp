#pragma once

#include <queue>
#include <thread>
#include <iostream>
#include <functional>
#include <future>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "logger.hpp"
using namespace std;

class ThreadPool
{
private:
    int m_nThreads;       //最大线程数
    float m_minRatio;     //收缩界限
    float m_maxRatio;     //扩张界限
    int m_tasksQueueSize; //队列上限
    bool m_pool;      //线程池开关

    int m_curThreads;
    atomic<thread::id> m_taskID;
    thread::id m_mainID;

    queue<function<void()>> m_tasksQueue;
    queue<thread *> m_threads;
    mutex m_mutexTasksQueue;

    mutex m_mutexThread;
    thread *m_adaptive_thread;

private:
    ThreadPool();
    ~ThreadPool();

public:
    void init(int nThreads, float minRatio, float maxRatio, int tasksQueueSize);
    static ThreadPool *get_instance();
    void push_task(function<void()> task); 
    void close();
    void task();

private:
    void m_adaptive_pool();
};