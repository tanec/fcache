#include <sys/types.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <memory.h>
#include <netinet/tcp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <netdb.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "sockutil.h"

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif

inline uint32_t
host2addr(const char *host)
{
  if (host == NULL || !*host || !strcmp(host,"*")) {
    return htonl(INADDR_ANY);
  } else {
    uint32_t tcp_ia = inet_addr(host);
    if (tcp_ia == INADDR_NONE) {
      struct hostent * hep;
      hep = gethostbyname(host);
      if ((!hep) || (hep->h_addrtype != AF_INET || !hep->h_addr_list[0])) {
        warn("fcgiev: cannot resolve host name %s", host);
        return -1;
      }
      if (hep->h_addr_list[1]) {
        warn("fcgiev: host %s has multiple addresses -- choose one explicitly", host);
        return -1;
      }
      tcp_ia = ((struct in_addr *) (hep->h_addr))->s_addr;
    }
    return tcp_ia;
  }
}

int
bind_tcp(const char *host, const in_port_t port, const int backlog)
{
  int  fd;
  uint32_t tcp_ia;
  struct sockaddr_in addr;

  if ((tcp_ia = host2addr(host)) < 0) return -1;
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return -1;
  } else {
    int flag = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag)) < 0) {
      warn("bind_tcp: can't set SO_REUSEADDR.");
      return -1;
    }
  }

  memset((char *)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = tcp_ia;
  addr.sin_port = htons(port);

  if(bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    perror("bind_tcp: bind");
    return -1;
  }

  if(listen(fd, backlog) < 0) {
    perror("bind_tcp: listen");
    return -1;
  }

  return fd;
}


inline static int
_build_un(const char *bindPath, struct sockaddr_un *servAddrPtr, int *servAddrLen)
{
  int bindPathLen = strlen(bindPath);
  if(bindPathLen > sizeof(servAddrPtr->sun_path))
    return -1;
  memset((char *) servAddrPtr, 0, sizeof(*servAddrPtr));
  servAddrPtr->sun_family = AF_UNIX;
  memcpy(servAddrPtr->sun_path, bindPath, bindPathLen);
  *servAddrLen = sizeof(servAddrPtr->sun_family) + bindPathLen;
  return 0;
}

int
bind_unix(const char *bindPath, const int backlog)
{
  int fd, servLen;
  struct sockaddr_un addr;

  if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) return -1;

  unlink(bindPath);
  if(_build_un(bindPath, &addr, &servLen)) {
    warn("bind_unix: listening socket's path name is too long.");
    return -1;
  }

  if(bind(fd, (struct sockaddr *) &addr, servLen) < 0) {
    perror("bind_unix: bind");
    return -1;
  }

  if(listen(fd, backlog) < 0) {
    perror("bind_unix: listen");
    return -1;
  }

  return fd;
}
