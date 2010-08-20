#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thread.h"

void
create_worker(void *(*func)(void *), void *arg)
{
  pthread_t       thread;
  pthread_attr_t  attr;
  int             ret;

  pthread_attr_init(&attr);

  if ((ret = pthread_create(&thread, &attr, func, arg)) != 0) {
    fprintf(stderr, "Can't create thread: %s\n", strerror(ret));
    exit(1);
  }
}
