#include "logger.hpp"

bool Logger::m_log = true;
Logger::Logger()
{
    m_logNums = 0;
    this->m_thread = new thread(mem_fn(&Logger::async_write), this);
}

void Logger::init(const char *dir, int maxLogs)
{
    m_maxLogs = maxLogs;
    m_dir = dir;
    string file;
    stringstream time_str;
    time_t t = chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time_str << std::put_time(std::localtime(&t), "%Y-%m-%d %X");
    if (-1==access(dir, 0) )
    {
        if(-1==mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO))
        {
            cerr<<"Could not create directory"<<endl;
            exit(1);
        }
    }
    file.append(dir);
    file.append("/");
    file.append(time_str.str());
    file.append(".txt");
    this->logfile = ofstream(file);
}
Logger *Logger::get_instance()
{
    static Logger instance;
    return &instance;
}

void Logger::close()
{
    m_log = false;
}
void Logger::async_write()
{
    while (m_log)
    {
        if (infoQueue.empty())
            continue;
        else
        {
            m_queue.lock();
            logfile << infoQueue.front() << endl;
            m_logNums++;
            infoQueue.pop();
            m_queue.unlock();
            if (m_logNums > m_maxLogs)
            {
                logfile.close();
                init(m_dir, m_maxLogs);
                m_logNums = 0;
            }
        }
    }
}

Logger::~Logger()
{
    while (infoQueue.empty() != true)
    {
        logfile << infoQueue.front() << endl;
        infoQueue.pop();
    }
    m_thread->join();
    delete m_thread;

    string info;
    stringstream time_str;
    time_t t = chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time_str << std::put_time(std::localtime(&t), "%Y-%m-%d %X");
    info.append(time_str.str());
    info.append("[info]\t");
    info.append("Logger close successfully");
    logfile << info << endl;
    logfile.close();
}
