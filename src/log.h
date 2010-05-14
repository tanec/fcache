#ifndef LOG_H
#define LOG_H

typedef int err_t;

#define MAX_ERROR_NUM   2048
#define LINEFEED_SIZE        1

void log_stderr(err_t err, const char *fmt, ...);

#endif // LOG_H
