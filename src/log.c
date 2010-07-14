#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include "log.h"

static FILE *log = NULL;
log_level_t default_level = DEBUG;
char *log_file = NULL;

void
tlog(log_level_t level, const char *fmt, ...)
{
  if (level >= default_level) {
    if (log == NULL) {
      log = fopen(log_file, "a");
    }
    if (log != NULL) {
      time_t rawtime;
      struct tm * timeinfo;
      char strtime[80];

      time(&rawtime);
      timeinfo = localtime(&rawtime);
      strftime(strtime, 80, "[%F %T]", timeinfo);
      fprintf(log, "%s ", strtime);

      va_list args;
      va_start(args, fmt);
      vfprintf(log, fmt, args);
      va_end(args);
      fprintf(log, "\n");
      fflush(log);
    }
  }
}
