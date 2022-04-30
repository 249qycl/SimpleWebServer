#include "argparse.hpp"
#include "logger.hpp"
#include "sql.hpp"
#include "reactor.hpp"
#include "thread_pool.hpp"

int main(int argc, char *argv[])
{
    ArgumentParser argparser(argc, argv);
    argparser.add_argument("-tcpport", 8000);
    argparser.add_argument("-tcplisten", 64);
    argparser.add_argument("-epsize", 64);
    argparser.add_argument("-maxlogs", 5000);
    argparser.add_argument("-logdir", "log");
    argparser.add_argument("-sqlhost", "localhost");
    argparser.add_argument("-sqluser", "root");
    argparser.add_argument("-sqlpass", "sql");
    argparser.add_argument("-sqldb", "webdata");
    argparser.add_argument("-sqlpoolsize", 16);
    argparser.add_argument("-threadnum", 16);
    argparser.add_argument("-threadmin", 0.2f);
    argparser.add_argument("-threadmax", 0.8f);
    argparser.add_argument("-threadqueue", 64);

    argparser.parse_args();

    Logger::get_instance()->init(argparser.args_str["logdir"], argparser.args_int["maxlogs"]);
    SQLPool::get_instance()->init(argparser.args_str["sqlhost"], argparser.args_str["sqluser"], argparser.args_str["sqlpass"], argparser.args_str["sqldb"], argparser.args_int["sqlpoolsize"]);
    ThreadPool::get_instance()->init(argparser.args_int["threadnum"], argparser.args_float["threadmin"], argparser.args_float["threadmax"], argparser.args_int["threadqueue"]);
    Reactor *web = new Reactor;

    web->init_server(argparser.args_int["tcpport"], argparser.args_int["tcplisten"], argparser.args_int["epsize"]);
    while(1);
    web->close_reactor();
    ThreadPool::get_instance()->close();
    SQLPool::get_instance()->close();
    Logger::get_instance()->close();
    while(1);
    return 0;
}