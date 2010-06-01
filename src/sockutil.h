#ifndef SOCKUTIL_H
#define SOCKUTIL_H

#include <sys/un.h>
#include <netinet/in.h>

typedef union {
  struct sockaddr_un un;
  struct sockaddr_in in;
} sau_t;

inline uint32_t host2addr(const char *);
int bind_tcp(const char *, const in_port_t, const int);
int bind_unix(const char *, const int);

#endif // SOCKUTIL_H
