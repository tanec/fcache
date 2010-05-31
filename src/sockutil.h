#ifndef SOCKUTIL_H
#define SOCKUTIL_H

#include <sys/un.h>
#include <netinet/in.h>

typedef union {
  struct sockaddr_un un;
  struct sockaddr_in in;
} sau_t;

int sockutil_bind(const char *, int, sau_t *);

#endif // SOCKUTIL_H
