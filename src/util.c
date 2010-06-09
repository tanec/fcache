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
    fprintf(stderr, "open(%s, O_RDONLY) failed!");
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
    fprintf(stderr, "mmap(%p, %d, %d, %d, %d, %d) failed!", 0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    return false;
  }
  if (close(fd) == -1) {
    fprintf(stderr, "close(%d) failed!", fd);
    munmap(ma->data, ma->len);
    return false;
  }
  return true;
}

int
mmap_close(mmap_array_t *ma)
{
  return munmap(ma->data, ma->len);
}

uint64_t
current_time_millis()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec*1000+(uint64_t)tv.tv_usec/1000;
}
