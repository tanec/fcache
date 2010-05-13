#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include "util.h"

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
