#include "thread_pool.hpp"

void ThreadPool::init(int nThread, float minRatio, float maxRatio, int tasksQueueSize)
{
    m_minRatio = minRatio;
    m_maxRatio = maxRatio;
    m_tasksQueueSize = tasksQueueSize;
    m_curThreads = 0;

    m_nThreads = nThread;
    m_pool = true;
    thread *th = new thread(mem_fn(&ThreadPool::task), this);
    m_threads.push(th);
    m_adaptive_thread = new thread(mem_fn(&ThreadPool::m_adaptive_pool), this);
    m_mainID = m_adaptive_thread->get_id();
    m_taskID = m_mainID;
}
ThreadPool *ThreadPool::get_instance()
{
    static ThreadPool instance;
    return &instance;
}

void ThreadPool::push_task(function<void()> task)
{
    while (m_tasksQueue.size() >= m_tasksQueueSize);
    m_mutexTasksQueue.lock();
    m_tasksQueue.push(task);
    m_mutexTasksQueue.unlock();
}

void ThreadPool::task()
{
    while (1)
    {
        unique_lock<mutex> lck(m_mutexTasksQueue);
        if (m_tasksQueue.empty())
        {
            if (m_taskID == this_thread::get_id())
            {
                m_taskID = m_mainID;
                break;
            }
            else
                continue;
        }
        function<void()> func = m_tasksQueue.front();
        m_tasksQueue.pop();
        lck.unlock();
        ++m_curThreads;
        func();
        --m_curThreads;
    }
}
void ThreadPool::close()
{
    m_pool = false;
}

void ThreadPool::m_adaptive_pool()
{
    while (1)
    {
        if (m_pool == true)
        {
            if (m_threads.size() > 1 && m_taskID == m_mainID && m_curThreads / (m_threads.size() + 0.01) < m_minRatio)
            {
                //缩容
                thread *th = m_threads.front();
                m_threads.pop();
                m_taskID = th->get_id();
                th->join();
                delete th;
            }
            else if (m_threads.size() < m_nThreads && m_curThreads / (m_threads.size() + 0.01) > m_maxRatio)
            { //扩容
                thread *th = new thread(mem_fn(&ThreadPool::task), this);
                m_threads.push(th);
            }
        }
        else
        {
            while (m_threads.empty() == false)
            {
                if (m_taskID == m_mainID)
                {
                    thread *th = m_threads.front();
                    m_taskID = th->get_id();
                    th->join();
                    delete th;
                    m_threads.pop();
                }
            }
            break;
        }

        auto fu = chrono::system_clock::now() + chrono::milliseconds(500);
        while (chrono::system_clock::now() < fu)
        {
            this_thread::yield();
        }
    }
}
ThreadPool::ThreadPool()
{
}
ThreadPool::~ThreadPool()
{
    m_adaptive_thread->join();
    delete m_adaptive_thread;
    LOG_INFO("ThreadPool close successfully");
}
