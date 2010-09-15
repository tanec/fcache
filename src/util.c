#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

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

static inline void *
myrealloc(void *ptr, size_t size)
{
  if(ptr)
    return realloc(ptr, size);
  else
    return malloc(size);
}

static inline size_t
write_tbuf(tbuf *dest, void *src, size_t size)
{
  dest->data = myrealloc(dest->data, dest->len + size);
  if (dest->data) {
    memcpy(&(dest->data[dest->len]), src, size);
    dest->len += size;
  }
  return size;
}

bool
tbuf_read(tbuf *buf, const char *file)
{
  int fd;
  struct stat sb;
  bool ret = true;

  fd = open(file, O_RDONLY);
  if (fd == -1) {
    //fprintf(stderr, "open(%s, O_RDONLY) failed!\n", file);
    return false;
  }
  if (fstat(fd, &sb) == -1) {
    ret = false;
    goto close_fd;
  }
  if (!S_ISREG(sb.st_mode)) {
    ret = false;
    goto close_fd;
  }
  if (sb.st_size < 1) {
    ret = false;
    goto close_fd;
  }

  do {
    ssize_t bytes = 0, sz = 8192;
    char buffer[sz];
    if ((bytes = read(fd, buffer, sz)) < 1) {
      break;
    }

    write_tbuf(buf, buffer, bytes);
  } while(1);

 close_fd:
  if (close(fd) == -1) {
    fprintf(stderr, "close(%d) failed!", fd);
    return false;
  }
  return ret;
}

void
tbuf_close(tbuf *buf)
{
  if (buf) {
    if (buf->data) free(buf->data);
  }
}

uint64_t
current_time_millis()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec*1000+(uint64_t)tv.tv_usec/1000;
}

uint64_t
current_time_micros()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec*1000*1000+tv.tv_usec;
}

static size_t
write_memory(void *dest, void *src, size_t size)
{
  tbuf *arr = (tbuf *)dest;

  arr->data = myrealloc(arr->data, arr->len + size + 1);
  if (arr->data) {
    memcpy(&(arr->data[arr->len]), src, size);
    arr->len += size;
    arr->data[arr->len] = '\0';
  }
  return size;
}

bool
tcp_read(tbuf *data, const char *host, uint16_t port, char *output)
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

  struct timeval nTimeOut;
  nTimeOut.tv_sec = 20;
  nTimeOut.tv_usec = 0;

  setsockopt(sock,SOL_SOCKET ,SO_RCVTIMEO,(char *)&nTimeOut,sizeof(struct timeval));
  setsockopt(sock,SOL_SOCKET ,SO_SNDTIMEO,(char *)&nTimeOut,sizeof(struct timeval));

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

void
strtolower(char *str)
{
  if (str == NULL) return;
  char *s = str;
  do {
    if (isupper(*s)) *s = tolower(*s);
  } while(*(++s)!='\0');
}

bool
ext_gunzip(tbuf *dest, const void* src, size_t count) {
    pid_t pid;
    int wp[2], rp[2]; //0:read, 1:write
    ssize_t rlen;
#define BLEN 8192
    char buf[BLEN];
    

    if (pipe(wp) == -1) { return false; }
    if (pipe(rp) == -1) { close(wp[0]); close(wp[1]); return false; }
 
    pid = fork();
    if (pid < 0) {
      close(rp[0]); close(rp[1]);
      close(wp[0]); close(wp[1]);
      return false;
    } else if (pid==0) {
      //child
      close(wp[1]);
      close(rp[0]);
      dup2(wp[0], STDIN_FILENO);
      dup2(rp[1], STDOUT_FILENO);
      close(wp[0]);
      close(rp[1]);
      if (execlp("/bin/gzip", "gzip", "-cdf", (char *)NULL) == -1) {
	perror("exec gzip");
      }
      exit(0);
    }else {
      //parent
      close(wp[0]);
      close(rp[1]);
      //write to stdin
      
      write(wp[1], src, count);
      close(wp[1]);
      //read from stdout
      do {
	rlen=read(rp[0], buf, BLEN);
	write_memory(dest, buf, rlen);
      } while(rlen > 0);
      close(rp[0]);
      int status;
      waitpid(pid, &status, 0);
      return true;
    }
    return false;
#undef BLEN
}
