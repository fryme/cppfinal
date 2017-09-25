#ifndef _WEB_SERVER_COMMON_H_
#define _WEB_SERVER_COMMON_H_

#include <stdexcept>
#include <random>
#include <fstream>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "logging.h"
#include "config.h"

const size_t BUFFER_SIZE = 256;

#define CHECK(e, str) if (e < 0) { std::clog << str << ":" << strerror(errno) << std::endl; return e; }
#define CHECK_THROW(e, str) if (e < 0) { std::string err(str); err += ": "; err += strerror(errno); std::clog << str << std::endl; throw std::runtime_error(err); }

inline std::string ReadFrom(int fd, bool nonBlock = false)
{
    std::string data;

    while (true)
    {
        std::string buffer(1024, '\0');
        int count = recv(fd, &buffer[0], buffer.size(), 0);

        if (count <= 0)
        {
            if (count < 0)
                throw std::runtime_error("Read error ocurred!");

            break;
        }

        buffer.resize(count);
        data += buffer;

        //std::clog << "Read: " << buffer << " " << count << std::endl;

        if (nonBlock)
            break;
    }

    return data;
}

inline ssize_t ReadFd(int sock, int* fd, std::string& buffer)
{
    ssize_t size;

    if (fd)
    {
        struct msghdr   msg;
        struct iovec    iov;
        union {
            struct cmsghdr  cmsghdr;
            char        control[CMSG_SPACE(sizeof (int))];
        } cmsgu;
        struct cmsghdr* cmsg;

        iov.iov_base = &buffer[0];
        iov.iov_len = buffer.length();

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
        size = recvmsg (sock, &msg, 0);

        if (size < 0)
        {
            std::clog << "ReadFd: error in recvmsg: " << strerror(errno) << std::endl;
            throw std::runtime_error("Error in recvmsg");
        }

        //std::clog << "ReadFd: read size: " << size << std::endl;

        if (size == 0)
            return size;

        buffer.resize(size);

        cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int)))
        {
            if (cmsg->cmsg_level != SOL_SOCKET)
            {
                std::clog << "ReadFd: invalid cmsg_level " << cmsg->cmsg_level << std::endl;
                exit(1);
            }

            if (cmsg->cmsg_type != SCM_RIGHTS)
            {
                std::clog << "ReadFd: invalid cmsg_type: " << cmsg->cmsg_type << std::endl;
                exit(1);
            }

            *fd = *((int *) CMSG_DATA(cmsg));
            //std::clog << "ReadFd: received fd: " << *fd << std::endl;
        }
        else
        {
            *fd = -1;
        }
    }

    return size;
}

inline size_t WriteTo(int sock, const std::string& message, int fd = -1)
{
    ssize_t size;
    struct msghdr msg;
    struct iovec iov;
    union {
        struct cmsghdr  cmsghdr;
        char        control[CMSG_SPACE(sizeof (int))];
    } cmsgu;
    struct cmsghdr  *cmsg;

    iov.iov_base = (void *)message.c_str();
    iov.iov_len = message.length();

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (fd != -1)
    {
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof (int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        //std::clog << "Write: passing fd: " << fd << std::endl;
        *((int *) CMSG_DATA(cmsg)) = fd;
    }
    else
    {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        //std::clog << "Write: not passing fd" << std::endl;
    }

    size = sendmsg(sock, &msg, 0);
    CHECK(size, "sendmsg");
    return size;
}

inline std::vector<std::string> ReadWords()
{
    std::vector<std::string> words;
    std::string dictPath(HOME_PATH);
    dictPath += "word_dict.txt";
    std::fstream stream(dictPath);
    std::string word;
    while (std::getline(stream, word))
    {
        words.push_back(word);
    }

    return words;
}

inline std::string SelectRandomWords(const std::vector<std::string>& words, int number)
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::string result;
    std::uniform_int_distribution<int> uni(0, words.size());

    while (true)
    {
        result += words[uni(rng)];
        result += " ";
        if (--number == 0)
            break;
    }

    return std::string(result.begin(), result.end() - 1);
}

#endif // _WEB_SERVER_COMMON_H_
