#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>

pid_t daemonize(int, int);
void md5sum(const void *, int, char *);

#endif // UTIL_H
