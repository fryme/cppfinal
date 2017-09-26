#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

#include <string>
#include <vector>
#include <map>
#include <list>
#include <sstream>

#include <arpa/inet.h>

#include <ev++.h>

#include "logging.h"
#include "common.h"
#include "config.h"
#include "server_worker.h"

struct ServerConfig
{
    std::string host;
    uint32_t port = 0;
    std::string directory;

    std::string ToString() const
    {
        std::string result;
        result += host;
        result += ":";
        result += std::to_string(port);
        result += " ";
        result += directory;
        return result;
    }
};

std::ostream& operator<<(std::ostream& stream, const ServerConfig& config)
{
    stream << "config: " << config.ToString() << std::endl;
    return stream;
}

void ReadAsync(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    char buffer[BUFFER_SIZE];
    ssize_t read;

    if(EV_ERROR & revents)
    {
        std::clog << "got invalid event" << std::endl;
        return;
    }

    //std::string data = ReadFrom(watcher->fd);
    // Receive message from client socket
    read = recv(watcher->fd, buffer, BUFFER_SIZE, 0);

    if(read < 0)
    {
        std::clog << "read error";
        return;
    }

    if(read == 0)
    {
        // Stop and free watchet if client socket is closing
        ev_io_stop(loop,watcher);
        free(watcher);
        std::clog << "peer might closing" << std::endl;
        //LogInfo("%d client(s) connected.\n", total_clients);
        return;
    }
    else
    {
        std::clog << "message:" << buffer << std::endl;
    }

    /*
    LogInfo("message:\n");
    LogInfo(data.c_str());
    */

    // Send message back to the client
    //send(watcher->fd, data.c_str(), data.size(), 0);
    send(watcher->fd, buffer, read, 0);
    bzero(buffer, read);
}

class WebServer
{
public:

    bool StartEventLoop(const ServerConfig& config)
    {
        std::clog << "Starting main event loop" << std::endl;

        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        CHECK_THROW(serverSocket, "socket");

        sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(config.port);
        // TODO: Rework!
        sa.sin_addr.s_addr = inet_addr(config.host.c_str());

        // Reuse port
        const int enable = 1;
        CHECK_THROW(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)), "setsockopt");

        CHECK_THROW(bind(serverSocket, (sockaddr*)&sa, sizeof(sa)), "bind");
        CHECK_THROW(listen(serverSocket, SOMAXCONN), "listen");

        struct ev_loop *loop = EV_DEFAULT;

        /*
        ev_io_init(&m_ioWatcher, [&](struct ev_loop *loop, struct ev_io *watcher, int revents)
        {
            AcceptCb(loop, watcher, revents);
        }, serverSocket, EV_READ);
        */

        m_ioWatcher.set<WebServer, &WebServer::AcceptConnection>(this);
        m_ioWatcher.start(serverSocket, ev::READ);

        //ev_io_start(loop, &m_ioWatcher);
        std::clog << "Starting main event last step" << std::endl;

        ev_run(loop, 0);
    }

    int StartWorkers()
    {
        std::clog << "Starting workers..." << std::endl;

        m_workers.reserve(MaxNumberOfWorkers);
        for (size_t w = 0; w < MaxNumberOfWorkers; ++w)
        {
            m_workers.push_back(Worker());
            m_workers.back().Start();
        }

        std::clog << "All workers started" << std::endl;
    }

private:

    void AcceptConnection(ev::io& watcher, int revents)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        if(EV_ERROR & revents)
        {
            std::clog << "Got invalid event" << std::endl;
            return;
        }

        // Accept client request
        int client = accept(watcher.fd, (struct sockaddr *)&client_addr, &client_len);
        CHECK_THROW(client, "accept")

        std::clog << "New client received" << std::endl;

        // Find worker
        Worker* w = nullptr;
        if (FindWorker(w))
        {
            // Send work to worker
            CHECK_THROW(w->SendSocket(client), "SendSocket");
            close(client);
        }
        else
        {
            std::clog << "Worker not found" << std::endl;
        }
    }

    bool FindWorker(Worker*& worker)
    {
        std::map<int, std::list<Worker*>> statusList;
        try
        {
            for (Worker &w : m_workers)
            {
                int status = 0;
                if (w.GetStatus(status) == 0)
                    statusList[status].push_back(&w);
            }

            std::clog << "Current status:" << std::endl;
            for (const auto& status: statusList)
            {
                std::stringstream str;
                str << status.first << " - ";
                for (const auto& w : status.second)
                    str << *w << " ";

                std::clog << str.str().c_str() << std::endl;
            }
        }
        catch(...)
        {
            return false;
        }

        worker = *statusList.begin()->second.begin();
        return true;
    }

    std::string GetCurrentPath()
    {
        /*
        char buffer[BUFFER_SIZE];
        sprintf(buffer, "/proc/%d/exe", getpid());
        readlink(buffer, )
        */
    }

private:
    ev::io m_ioWatcher;
    std::vector<Worker> m_workers;
};

#endif // _WEB_SERVER_H_
