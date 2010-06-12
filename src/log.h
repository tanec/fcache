#ifndef LOG_H
#define LOG_H

typedef int err_t;

#define MAX_ERROR_NUM   2048
#define LINEFEED_SIZE        1

typedef enum {
  DEBUG = 0,
  INFO  = 1,
  WARN  = 2,
  ERROR = 3,
  FATAL = 4
} log_level_t;

void tlog(log_level_t level, const char *fmt, ...);

extern log_level_t default_level;
extern char *log_file;
extern int consolelog;

#endif // LOG_H
