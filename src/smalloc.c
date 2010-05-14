#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smalloc.h"

#define LENGTH_SIZE sizeof(size_t)

static size_t used_memory = 0;

static void
smalloc_die(size_t size)
{
  fprintf(stderr, "smalloc: Out of memory trying to allocate %zu bytes\n", size);
  fflush(stderr);
  abort();
}

void *
smalloc(size_t size)
{
  void *ptr = malloc(size+LENGTH_SIZE);
  if (!ptr) smalloc_die(size);
  *((size_t*)ptr) = size;
  used_memory += size+LENGTH_SIZE;
  return (char*)ptr+LENGTH_SIZE;
}

void *
srealloc(void *ptr, size_t size)
{
  void *realptr;
  size_t oldsize;
  void *newptr;

  if (ptr == NULL) return smalloc(size);
  realptr = (char*)ptr-LENGTH_SIZE;
  oldsize = *((size_t*)realptr);
  newptr = realloc(realptr,size+LENGTH_SIZE);
  if (!newptr) smalloc_die(size);

  *((size_t*)newptr) = size;
  used_memory -= oldsize;
  used_memory += size;
  return (char*)newptr+LENGTH_SIZE;
}

void
sfree(void *ptr)
{
  void *realptr;
  size_t oldsize;
  if (ptr == NULL) return;
  realptr = (char*)ptr-LENGTH_SIZE;
  oldsize = *((size_t*)realptr);
  used_memory -= oldsize+LENGTH_SIZE;
  free(realptr);
}

char *
sstrdup(const char *s)
{
  size_t l = strlen(s)+1;
  char *p = smalloc(l);

  memcpy(p,s,l);
  return p;
}

size_t
smalloc_used_memory(void)
{
  return used_memory;
}
