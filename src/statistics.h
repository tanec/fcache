#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdint.h>
#include <time.h>

typedef struct {
  uint64_t num;
  uint64_t time;
  uint64_t max_time;
} stat_request_complete_t;

typedef struct {
  uint64_t total_num; // enter number
  stat_request_complete_t success;
  stat_request_complete_t notfound;
} stat_item_t;

int touch_timespec(struct timespec *);
uint64_t time_diff(struct timespec, struct timespec);
void stat_add(stat_request_complete_t *, uint64_t);

#endif // STATISTICS_H
