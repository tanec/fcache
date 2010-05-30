#ifndef LOG_H
#define LOG_H

typedef int err_t;

#define MAX_ERROR_NUM   2048
#define LINEFEED_SIZE        1

typedef enum {
  DEBUG = 0,
  INFO  = 1,
  ERROR = 2,
  FATAL = 3
} log_level_t;

void tlog(log_level_t level, const char *fmt, ...);

#endif // LOG_H
