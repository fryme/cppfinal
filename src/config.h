#ifndef _CONFIG_H_
#define _CONFIG_H_

#define HOME_PATH "/home/box/"

// Logs
#define LOG_FILE_ENABLED true
#define USE_FIXED_LOG_FILE_NAME true
#define LOG_FILE_NAME "log.txt"
#define PATH_TO_LOGS_FOLDER HOME_PATH"logs/"

// Workers
//#define WorkerPath ""
const uint32_t MaxNumberOfWorkers = 2;

#endif // _CONFIG_H_
