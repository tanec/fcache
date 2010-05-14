#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include "util.h"
#include "md5.h"

pid_t
daemonize(int dochdir, int doclose)
{
  pid_t pid, sid;

  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "fork() < 0");
    exit(EXIT_FAILURE);
  } else if (pid > 0) {
    exit(EXIT_SUCCESS);
  }
  // prepare daemon
  umask(0);

  if ((sid = setsid()) < 0) {
    fprintf(stderr, "setsid() < 0");
    exit(EXIT_FAILURE);
  }
  if (dochdir && (chdir("/")) < 0) {
    fprintf(stderr, "chdir(char*) < 0");
    exit(EXIT_FAILURE);
  }
  if (doclose) {
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
  }

  return sid;
}

void
md5sum(const void *ptr, int size, char *buf)
{
  int i;
  unsigned char digest[16];
  md5_state_t ms;

  md5_init(&ms);
  md5_append(&ms, (md5_byte_t *)ptr, size);
  md5_finish(&ms, (md5_byte_t *)digest);

  char *wp = buf;
  for(i = 0; i < 16; i++){
    wp += sprintf(wp, "%02x", digest[i]);
  }
  *wp = '\0';
}
