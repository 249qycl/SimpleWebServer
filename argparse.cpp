#include "argparse.hpp"


ArgumentParser::ArgumentParser(int argc, char **argv)
{
    m_argc = argc;
    m_argv = argv;
}
ArgumentParser::~ArgumentParser() {}

void ArgumentParser::add_argument(const char *argname, int default_value)
{
    string name(argname);
    args_int[name.substr(1)] = default_value;
}
void ArgumentParser::add_argument(const char *argname, float default_value)
{
    string name(argname);
    args_float[name.substr(1)] = default_value;
}
void ArgumentParser::add_argument(const char *argname, const char* default_value)
{
    string name(argname);
    args_str[name.substr(1)] = default_value;
}

void ArgumentParser::parse_args()
{
    for (int i = 1; i < m_argc; i += 2)
    {
        string arg(m_argv[i]);
        if (arg.find("-") != string::npos)
        {
            if (args.find(arg.substr(1)) == args.end())
            {
                m_usuage();
                exit(0);
            }
            else
            {
                if (args_int.find(arg.substr(1)) != args_int.end())
                    args_int[arg.substr(1)] = atoi(m_argv[i + 1]);
                else if (args_float.find(arg.substr(1)) != args_float.end())
                    args_float[arg.substr(1)] = atof(m_argv[i + 1]);
                else if (args_str.find(arg.substr(1)) != args_str.end())
                    args_str[arg.substr(1)] = m_argv[i + 1];
            }
        }
    }
}

void ArgumentParser::m_usuage()
{
    cout << "Usage: " << endl;
    cout << "-port"
         << "\t"
         << "端口号：9000～9999" << endl;
    cout << "-log"
         << "\t"
         << "日志写入方式：同步0、异步1" << endl;
    cout << "-trig"
         << "\t"
         << "触发模式" << endl;
}
