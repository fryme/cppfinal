#include "log.h"
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "config.h"

extern Settings MySettings;

Log::Log(std::string ident, int facility, int logFileFd)
    : m_logFileFd(logFileFd)
{
    facility_ = facility;
    m_priority = LOG_INFO;
    strncpy(ident_, ident.c_str(), sizeof(ident_));
    ident_[sizeof(ident_)-1] = '\0';

    openlog(ident_, LOG_PID | LOG_CONS, LOG_USER);
    sprintf(m_prefix, "[%s] ", MySettings.name.c_str());

    if (LOG_FILE_ENABLED)
    {
        if (m_logFileFd == -1)
            CreateLogFile();
        else
            lseek(m_logFileFd, 0, SEEK_END);
    }
}

Log::~Log()
{
    close(m_logFileFd);
}

std::string GetCurrentTime()
{
    std::time_t now = std::time(NULL);
    std::tm* ptm = std::localtime(&now);
    std::string buffer(8, '\0');
    std::strftime(&buffer[0], 32, "%H:%M:%S", ptm);
    return buffer;
}

std::string Log::GetFileName(const std::string& tmpDir) const
{
    std::string tmpName(tmpDir);
    std::time_t now = std::time(NULL);
    std::tm* ptm = std::localtime(&now);
    char buffer[32];
    std::strftime(buffer, 32, "web_server_%d.%m.%Y_%H.%M.%S", ptm);
    tmpName += buffer;
    syslog(m_priority, "[%s] Log file name: %s", m_prefix, tmpName.c_str());
    return tmpName;
}

void Log::CreateLogFile()
{
    std::string fileName;
    if (USE_FIXED_LOG_FILE_NAME)
    {
        fileName = PATH_TO_LOGS_FOLDER;
        fileName += LOG_FILE_NAME;
        m_logFileFd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0777);
    }
    else
    {
        fileName = GetFileName(PATH_TO_LOGS_FOLDER);
        m_logFileFd = open(fileName.c_str(), O_CREAT | O_RDWR, 0777);
    }

    if (m_logFileFd < 0)
        syslog(m_priority, "[%s] Open err: %s", m_prefix, strerror(errno));
}

int Log::sync()
{
    if (m_buffer.length())
    {
        m_buffer.insert(0, m_prefix, strlen(m_prefix));
        syslog(m_priority, "%s", m_buffer.c_str());

        if (LOG_FILE_ENABLED)
        {
            std::string currentTime = GetCurrentTime();
            currentTime += " ";
            m_buffer.insert(0, currentTime);
            if (write(m_logFileFd, m_buffer.c_str(), m_buffer.length()) < 0)
                syslog(m_priority, "[%s] Write to log error: %s, %d", MySettings.name.c_str(), strerror(errno), m_logFileFd);
        }
        
        m_buffer.erase();
        m_priority = LOG_DEBUG; // default to debug for each message
    }
    return 0;
}

int Log::overflow(int c)
{
    if (c != EOF)
        m_buffer += static_cast<char>(c);
    else
        sync();

    return c;
}

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority)
{
    static_cast<Log *>(os.rdbuf())->m_priority = (int)log_priority;
    return os;
}
