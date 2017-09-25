#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <string>

#include <syslog.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "log.h"
#include <iostream>

inline int GetCurrentTid()
{
    return syscall(SYS_gettid);
}

/*
struct LogInfo
{
    std::stringstream m_stream;
    std::ostream& operator<<(std::string str)
    {
        m_stream << str;
        return stream;
    }

    ~LogInfo()
    {

    }
};
 */

/*
void LogInfo(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    // TODO: Rework it!
    std::string str(format);
    str.insert(0, "] ");
    str.insert(0, std::to_string(GetCurrentTid()));
//    str.insert(0, ":");
//    str.insert(0, std::to_string(getpid()));
    str.insert(0, "[");

    syslog(LOG_INFO, str.c_str(), args);
    va_end(args);
}

void LogError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    std::string str(format);
    str.insert(0, "] ");
    str.insert(0, std::to_string(GetCurrentTid()));
    str.insert(0, "[");
    syslog(LOG_ERR, str.c_str(), args);
    va_end(args);
}

*/

#endif // _LOGGING_H_