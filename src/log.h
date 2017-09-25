#ifndef _LOG_H_
#define _LOG_H_

#include <string>
#include <string.h>
#include <ostream>
#include <syslog.h>
#include <fstream>
#include <iostream>
#include <memory>

struct Settings;
extern Settings MySettings;

enum LogPriority
{
    kLogEmerg   = LOG_EMERG,   // system is unusable
    kLogAlert   = LOG_ALERT,   // action must be taken immediately
    kLogCrit    = LOG_CRIT,    // critical conditions
    kLogErr     = LOG_ERR,     // error conditions
    kLogWarning = LOG_WARNING, // warning conditions
    kLogNotice  = LOG_NOTICE,  // normal, but significant, condition
    kLogInfo    = LOG_INFO,    // informational message
    kLogDebug   = LOG_DEBUG    // debug-level message
};

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);

class Log : public std::basic_streambuf<char, std::char_traits<char> >
{
public:
    explicit Log(std::string ident, int facility, int logFileFd = -1);
    ~Log();

    int GetLogFileFd() const
    {
        return m_logFileFd;
    }

protected:
    int sync();
    int overflow(int c);

private:
    friend std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);
    std::string m_buffer;
    int facility_;
    int m_priority;
    char ident_[50];
    char m_prefix[256];

    void CreateLogFile();
    std::string GetFileName(const std::string& tmpDir) const;

    int m_logFileFd;

};

struct Settings
{
    std::string name;
    std::shared_ptr<Log> log;
    std::string homePath;
};

extern Settings MySettings;

inline void StartLogging()
{
    std::clog.rdbuf(MySettings.log.get());
}

inline void StopLogging()
{
    closelog();
}

#endif // _LOG_H_
