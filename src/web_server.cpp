#include <iostream>
#include <string>

#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <netinet/in.h>

#include <ev.h>

#include "logging.h"
#include "common.h"
#include "web_server.h"

#include "log.h"

Settings MySettings;// = {"", std::make_shared<Log>("web_server", LOG_INFO)};

int DaemonInit()
{
    pid_t pid;

    pid = fork();
    CHECK(pid, "fork");

    // Parent process terminates
    if (pid > 0)
        exit(EXIT_SUCCESS);


    CHECK(setsid(), "setsid");

    // No childs
    signal(SIGCHLD, SIG_IGN);
    // Config reread
    signal(SIGHUP, SIG_IGN);

    pid = fork();

    CHECK(pid, "second fork");

    // Parent process terminates
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // Set new file permissions
    umask(0);

    chdir("/");

    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
        close(x);
}

void PrintHelp(const char* name)
{
    std::cerr << "Usage " << name << " -h <ip> -p <port> -d <directory>" << std::endl;
}

ev_io stdout_watcher;

static void stdout_cb(EV_P_ ev_io* w, int revents)
{
    //ev_io_stop(EV_A_ w);
    syslog(LOG_INFO, "stdout_cb called");

    //ev_break(EV_A_ EVBREAK_ALL);
}

struct ServerConfig;

int main(int argc, char** argv)
{
    try
    {
        int c = 0;
        ServerConfig config;
        while ((c = getopt(argc, argv, ":h:p:d:")) != -1) {
            switch (c) {
                case 'h':
                    config.host = std::string(optarg);
                    break;
                case 'p':
                    config.port = atoi(optarg);
                    break;
                case 'd':
                    config.directory = std::string(optarg);
                    if (config.directory.back() != '/')
                        config.directory.push_back('/');
                    break;
                default:
                    PrintHelp(argv[0]);
                    return 0;
            }
        }

        if (config.directory.empty() || config.host.empty() || config.port == 0) {
            PrintHelp(argv[0]);
            return 0;
        }

        std::cout << "Started with " << config;
        DaemonInit();
        MySettings.name = "Server:" + std::to_string(getpid());
        MySettings.log = std::make_shared<Log>("web_server", LOG_INFO);

        MySettings.homePath = config.directory;
        StartLogging();

        std::clog << "Hello, i'm your web server!" << std::endl;
        std::clog << "Config: " << config << std::endl;

        try
        {
            WebServer server;
            server.StartWorkers();
            server.StartEventLoop(config);
        }
        catch(const std::exception& e)
        {
            std::clog << "Error: " << e.what() << std::endl;
        }

        std::clog << "Goodbye!" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::clog << "Error: " << e.what() << std::endl;
        //std::cerr << "Error: " << e.what() << std::endl;
    }
    catch(...)
    {
        std::clog << "Error: Unhandled exception" << std::endl;
        //std::cerr << "Unhandled exception" <<;
    }

    StopLogging();
    return 0;
}
