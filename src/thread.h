#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>

void create_worker(void *(*func)(void *), void *arg);

#endif // THREAD_H
