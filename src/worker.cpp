
#include <ev.h>

#include <thread>
#include <mutex>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "common.h"
#include "logging.h"
#include "protocol.h"
#include "http_parser.h"

std::mutex workerThreadsSync;
std::vector<std::thread> workerThreads;
Settings MySettings;

const char* HttpResponseExample = "\
HTTP/1.1 200 OK\r\n\
Content-Type: application/json; charset=utf-8\rn\n\
Date: Thu, 24 Aug 2017 18:06:40 GMT\rn\n\
Content-Length: 10\rn\n\r\n\r\n\
JavaScript";

const char* HttpNotFoundResponse = "\
HTTP/1.1 404 Not found\r\n\
Content-Type: application/json; charset=utf-8\rn\n\
Date: Thu, 24 Aug 2017 18:06:40 GMT\rn\n\
Content-Length: 10\rn\n\r\n\r\n\
NotFound";

/* Read client message */
void ReadHttpRequestCallback(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    if (EV_ERROR & revents)
    {
        std::clog << "Read error" << std::endl;
        return;
    }

    if (EV_READ & revents)
    {
        std::string message = ReadFrom(watcher->fd, true);
        std::clog << "<ReadHttpRequestCallback> message received: " << message << std::endl;

        WriteTo(watcher->fd, HttpResponseExample);
    }
}

/* Read server command */
void ReadCommandCallback(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    if (EV_ERROR & revents)
    {
        std::clog << "Read error" << std::endl;
        return;
    }

    if (EV_READ & revents)
    {
        try
        {
            //std::clog << "Read occured" << std::endl;
            std::string command(BUFFER_SIZE, '\0');
            int optionalFd = 0;
            ssize_t sizeRead = ReadFd(watcher->fd, &optionalFd, command);
            if (sizeRead == 0)
            {
                // When  a  stream  socket  peer  has performed an orderly shutdown, the return value
                // will be 0 (the traditional "end-of-file" return).
                //
                // We have to end our loop and terminate process

                std::clog << "Server closed connection, shutdown" << std::endl;
                ev_break(ev_default_loop(0), EVBREAK_ALL);
                return;
            }

            protocol::CommandData data;
            protocol::ParseMessage(command, data);

            //std::clog << "Command received: '" << command << "'" << std::endl;
            switch (data.cmd)
            {
                case protocol::Command::RequestStatus:
                {
                    std::string numberOfWorkingThreads;
                    {
                        workerThreadsSync.lock();
                        numberOfWorkingThreads = std::to_string(workerThreads.size());
                        workerThreadsSync.unlock();
                    }

                    std::string message;
                    protocol::CreateMessage(protocol::Command::SendStatus, message, numberOfWorkingThreads);
                    WriteTo(watcher->fd, message);
                    std::clog << "<RequestStatus> result: " << message << std::endl;
                    break;
                }
                case protocol::Command::SendTask:
                {
                    // Always comes with fd
                    //std::clog << "<SendTask> fd received: " << optionalFd << std::endl;

                    workerThreadsSync.lock();
                    workerThreads.push_back(std::thread([optionalFd]() {
                        //std::clog << "<SendTask> Worker thread started " << GetCurrentTid() << std::endl;
                        std::string message = ReadFrom(optionalFd, true);

                        //std::clog << "<ReadHttpRequestCallback> message received: " << message << std::endl;
                        try
                        {
                            http::Request request(message);
                            std::clog << std::hex << GetCurrentTid() << " <SendTask> resource requested: '"
                                      << request.GetResourcePath() << "'" << std::endl;

                            if (request.GetType() == http::Request::RequestType::Get)
                            {
                                //std::clog << "We received GET request" << std::endl;
                                const auto filepath = MySettings.homePath + request.GetResourcePath();

                                int fd = open(filepath.c_str(), O_RDONLY);

                                if (fd < 0)
                                {
                                    if (errno == EACCES || ENOENT)
                                    {
                                        http::Response response;
                                        response.AddResponseCode(404);

                                        const char* NotFoundBody = "<html><body>Not found!!!</body></html>";
                                        response.AddHeader({"Content-Length", std::to_string(strlen(NotFoundBody))});
                                        response.AddHeader({"Content-Type", "text/html"});

                                        response.AddData(NotFoundBody);

                                        WriteTo(optionalFd, response.GetRaw());
                                        std::clog << std::hex << GetCurrentTid() << "<SendTask> resource not found"<< std::endl;
                                    }
                                    else
                                    {
                                        http::Response response;
                                        response.AddResponseCode(400);

                                        const char* BadRequestBody = "<html><body>Bad request</body></html>";
                                        response.AddHeader({"Content-Length", std::to_string(strlen(BadRequestBody))});
                                        response.AddHeader({"Content-Type", "text/html"});

                                        response.AddData(BadRequestBody);

                                        WriteTo(optionalFd, response.GetRaw());
                                        std::clog << std::hex << GetCurrentTid() << "<SendTask> bad request"<< std::endl;
                                    }
                                }
                                else
                                {
                                    struct stat stat_buffer;
                                    fstat(fd, &stat_buffer);
                                    std::string buffer;
                                    buffer.resize(stat_buffer.st_size);
                                    read(fd, &buffer[0], buffer.size());

                                    http::Response response;
                                    response.AddResponseCode(200);
                                    response.AddHeader({"Content-Length", std::to_string(buffer.size())});
                                    response.AddHeader({"Content-Type", "text/html"});
                                    response.AddData(buffer);

                                    WriteTo(optionalFd, response.GetRaw());
                                    close(fd);
                                    close(optionalFd);
                                    std::clog << std::hex << GetCurrentTid() << " <SendTask> resource sent, size: " << buffer.size()<< std::endl;
                                }
                            }
                        }
                        catch(const std::exception& e)
                        {
                            std::clog << "HttpRequest exception: " << e.what() << std::endl;
                        }

                        //std::clog << "<SendTask> Worker thread ended " << GetCurrentTid() << std::endl;
                    }));

                    workerThreadsSync.unlock();

                    break;
                }

                default:
                    std::clog << "Unrecognized command" << std::endl;
            }
        }
        catch(const std::exception& e)
        {
            std::clog << "Error while receiving data: "  << e.what() << std::endl;
        }
    }
}

void ToUpper(std::string& str)
{
    str[0] = std::toupper(str[0]);
    std::string::size_type pos = str.find_first_of(' ');
    str[pos + 1] = std::toupper(str[pos + 1]);
}

int main(int argc, char** argv)
{
    try
    {
        MySettings.name = SelectRandomWords(ReadWords(), 1) + ":" + std::to_string(getpid());
        //ToUpper(MySettings.name);
        MySettings.log = std::make_shared<Log>("web_server", LOG_INFO, atoi(argv[1]));
        MySettings.homePath = argv[3];
        StartLogging();

        std::clog << "Hi, i'm ready!" << std::endl;

        int serverSocket = atoi(argv[2]);
        struct ev_loop* loop = ev_default_loop(0);
        struct ev_io serverCommandsIo;
        ev_io_init(&serverCommandsIo, ReadCommandCallback, serverSocket, EV_READ);
        ev_io_start(loop, &serverCommandsIo);
        ev_run(loop, 0);

        close(serverSocket);
    }
    catch(const std::exception& e)
    {
        std::clog << "Error: " << e.what() << std::endl;
    }

    std::clog << "Goodbye!" << std::endl;
    StopLogging();

    return 0;
}
