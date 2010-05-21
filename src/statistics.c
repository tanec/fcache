#include "statistics.h"

//1s=1000milliseconds=1000000microseconds=1000000000nanoseconds
int
touch_timespec(struct timespec *time)
{
  return clock_gettime(CLOCK_PROCESS_CPUTIME_ID, time);
}

uint64_t
time_diff(struct timespec start, struct timespec end)
{
  struct timespec temp;
  if ((end.tv_nsec-start.tv_nsec)<0) {
    temp.tv_sec = end.tv_sec-start.tv_sec-1;
    temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
  } else {
    temp.tv_sec = end.tv_sec-start.tv_sec;
    temp.tv_nsec = end.tv_nsec-start.tv_nsec;
  }
  return temp.tv_sec*1000+temp.tv_nsec/1000000;
}

void
stat_add(stat_request_complete_t *s, uint64_t time)
{
  s->num++;
  s->time+=time;
  s->max_time = time > s->max_time ? time : s->max_time;
}
