#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "log.h"
#include "settings.h"

static FILE *log = NULL;

void
tlog(log_level_t level, const char *fmt, ...)
{
  if (level >= cfg.log_level) {
    if (!cfg.daemon) {
      va_list args;
      va_start(args, fmt);
      vprintf(fmt, args);
      va_end(args);
      printf("\n");
      return;
    }
    if (log == NULL) {
      log = fopen(cfg.log_file, "a");
      if(log!=NULL) setvbuf(log, NULL, _IONBF, 0);
    }
    if (log != NULL) {
      va_list args;
      va_start(args, fmt);
      vfprintf(log, fmt, args);
      va_end(args);
      fprintf(log, "\n");
    }
  }
}
