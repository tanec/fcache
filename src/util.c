#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    fprintf(stderr, "open(%s, O_RDONLY) failed!\n", file);
    return false;
  }
  if (fstat(fd, &sb) == -1) {
    return false;
  }
  if (!S_ISREG(sb.st_mode)) {
    return false;
  }

  ma->len = sb.st_size;
  ma->data = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (ma->data == MAP_FAILED) {
    fprintf(stderr, "mmap(%p, %d, %d, %d, %d, %d) failed!\n", 0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
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

static inline void *
myrealloc(void *ptr, size_t size)
{
  if(ptr)
    return realloc(ptr, size);
  else
    return malloc(size);
}

static size_t
write_memory(void *dest, void *src, size_t size)
{
  mmap_array_t *arr = (mmap_array_t *)dest;

  arr->data = myrealloc(arr->data, arr->len + size + 1);
  if (arr->data) {
    memcpy(&(arr->data[arr->len]), src, size);
    arr->len += size;
    arr->data[arr->len] = '\0';
  }
  return size;
}

bool
tcp_read(mmap_array_t *data, const char *host, uint16_t port, char *output)
{
  int sock;
  struct sockaddr_in server;

  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    printf("Failed to create socket\n");
    return false;
  }

  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;               /* Internet/IP */
  server.sin_addr.s_addr = inet_addr(host);  /* IP address */
  server.sin_port = htons(port);             /* server port */

  if (connect(sock,
              (struct sockaddr *) &server,
              sizeof(server)) < 0) {
    printf("Failed to connect with server\n");
    return false;
  }

  size_t output_len = 0;
  ssize_t send_len  = 0, s;
  if (output!=NULL && (output_len=strlen(output))>0) {
    do {
      s = send(sock, output+send_len, output_len-send_len, MSG_DONTWAIT);
      if (s < 0) break;
      send_len += s;
    } while(send_len<output_len);
  }

  do {
    ssize_t bytes = 0;
#define BUFFSIZE 8192
    char buffer[BUFFSIZE];
    if ((bytes = recv(sock, buffer, BUFFSIZE, 0)) < 1) {
      break;
    }

    write_memory(data, buffer, bytes);
  } while(1);

  close(sock);
  return data->len > 0;
}
