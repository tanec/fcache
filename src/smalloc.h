#ifndef SMALLOC_H
#define SMALLOC_H

#include <sys/types.h>

void *smalloc(size_t);
void *srealloc(void *, size_t);
void sfree(void *);
char *sstrdup(const char *);
size_t smalloc_used_memory(void);

#endif // SMALLOC_H
