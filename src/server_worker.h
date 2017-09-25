#ifndef _WORKER_H_
#define _WORKER_H_

#include <string>
#include <array>
#include <thread>

#include <ev++.h>

#include "common.h"
#include "logging.h"
#include "protocol.h"

char WorkerPath[] = "/home/box/final/worker";
//ev::io m_ioWatcher; // For communication with worker

class Worker
{
public:

    int Start()
    {
        int sockets[2] = { 0, 0 };
        static const size_t ParentSocket = 0;
        static const size_t ChildSocket = 1;

        CHECK_THROW(socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), "socketpair");

        // Start worker process
        int pid = fork();

        if (pid == 0)
        {
            // Child
            close(sockets[ParentSocket]);
            char* args[5];
            args[0] = WorkerPath;

            extern Settings MySettings;
            char logsFileFdStr[BUFFER_SIZE];
            sprintf(logsFileFdStr, "%d", MySettings.log->GetLogFileFd());

            char childSocketStr[BUFFER_SIZE];
            sprintf(childSocketStr, "%d", sockets[ChildSocket]);

            args[1] = logsFileFdStr;
            args[2] = childSocketStr;
            args[3] = &MySettings.homePath[0];
            args[4] = nullptr;

            CHECK_THROW(execvp(WorkerPath, args), "execvp");
        }
        else if (pid > 0)
        {
            // Parent
            close(sockets[ChildSocket]);
            m_pid = pid;
            m_socket = sockets[ParentSocket];
            std::clog << "New worker started" << std::endl;
        }
        else
        {
            std::clog << "Error starting worker!" << std::endl;
            // throw
        }

        return 0;
    }

    int GetStatus(int& numberOfServingClients)
    {
        // Send request
        std::string message;
        protocol::CreateMessage(protocol::Command::RequestStatus, message);
        WriteTo(m_socket, message);

        // Read answer
        message = ReadFrom(m_socket, true);
        //std::clog << "GetStatus: we received " << message.c_str() << std::endl;
        protocol::CommandData data;
        protocol::ParseMessage(message, data);
        if (data.cmd != protocol::Command::SendStatus)
        {
            std::clog << "GetStatus: error! Wrong command!" << std::endl;
            return -1;
        }

        numberOfServingClients = atoi(data.payload.c_str());
        //std::clog << "GetStatus: " << numberOfServingClients << std::endl;
        return 0;
    }

    int SendSocket(int clientSocket)
    {
        // Send socket to worker
        std::string message;
        protocol::CreateMessage(protocol::Command::SendTask, message);
        ssize_t size = WriteTo(m_socket, message, clientSocket);
        //std::clog << "SendSocket: socket to send: " << clientSocket << " result sending: " << size << std::endl;

        // Worker will read data from socket and work with it
        return 0;
    }

    int Stop()
    {
        // Kill process

        // close sockets

        return 0;
    }

private:
    int m_pid = 0;

    // Socket to communicate with worker process
    int m_socket = 0;

    friend std::ostream& operator<<(std::ostream& stream, const Worker& w);
};

std::ostream& operator<<(std::ostream& stream, const Worker& w)
{
    stream << "W(" << w.m_pid << ")";
    return stream;
}

#endif // _WORKER_H_
