#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include "log.h"

static int logfd = -1;
log_level_t default_level = DEBUG;
char *log_file = NULL;

void
tlog(log_level_t level, const char *fmt, ...)
{
  if (level >= default_level) {
    if (logfd < 0 && log_file!=NULL) {
      logfd = open(log_file, O_WRONLY|O_CREAT|O_APPEND);
    }

    time_t rawtime;
    struct tm * timeinfo;
    char strtime[80];
#define LINENUM 8192
    char msg[LINENUM] = {0};

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(strtime, 80, "[%F %T]", timeinfo);

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, LINENUM, fmt, args);
    va_end(args);

    size_t len = strlen(strtime) + strlen(msg) + 3;
    char line[len];
    sprintf(line, "%s %s\n", strtime, msg);

    //    printf("%s", line);
    if (logfd > -1) write(logfd, line, strlen(line));
  }
}
