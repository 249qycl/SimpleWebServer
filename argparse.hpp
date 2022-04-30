#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>
using namespace std;

class ArgumentParser
{
private:
    map<string, int> args;
    int m_argc;
    char **m_argv;

public:
    ArgumentParser(int argc, char *argv[]);
    ~ArgumentParser();

public:
    map<string, int> args_int;
    map<string, float> args_float;
    map<string, const char*> args_str;

    void add_argument(const char *argname, int default_value);
    void add_argument(const char *argname, float default_value);
    void add_argument(const char *argname, const char *default_value);

    void parse_args();

private:
    void m_usuage();
};
