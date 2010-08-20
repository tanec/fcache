#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smalloc.h"
#include "util.h"

#define LENGTH_SIZE sizeof(size_t)

static size_t used_memory = 0;

static void *
smalloc_die(size_t size)
{
  fprintf(stderr, "smalloc: Out of memory trying to allocate %zu bytes\n", size);
  fflush(stderr);
  return NULL;
}

void *
smalloc(size_t size)
{
  void *ptr = malloc(size+LENGTH_SIZE);
  if (ptr == NULL) return smalloc_die(size);
  *((size_t*)ptr) = size;
  SYNC_ADD(&used_memory, size+LENGTH_SIZE);
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
  if (!newptr) return smalloc_die(size);

  *((size_t*)newptr) = size;
  SYNC_SUB(&used_memory, oldsize);
  SYNC_ADD(&used_memory, size);
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
  SYNC_SUB(&used_memory, oldsize+LENGTH_SIZE);
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
