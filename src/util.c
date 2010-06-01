#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
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

/* mmap() read - write */
bool
mmap_read(mmap_array_t *ma, const char *file)
{
  int fd;
  struct stat sb;

  fd = open(file, O_RDONLY);
  if (fd == -1) {
    return false;
  }
  if (fstat(fd, &sb) == -1) {
    return false;
  }
  if (!S_ISREG(sb.st_mode)) {
    //fprintf (stderr, "%s is not a file\n", argv[1]);
    return false;
  }

  ma->len = sb.st_size;
  ma->data = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (ma->data == MAP_FAILED) {
    return false;
  }
  if (close(fd) == -1) {
    munmap(ma->data, ma->len);
    return false;
  }
}

int
mmap_close(mmap_array_t *ma)
{
  return munmap(ma->data, ma->len);
}

uint64_t
current_time_millis()
{
  return 0; //TODO
}
