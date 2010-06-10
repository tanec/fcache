#include "statistics.h"

void
stat_init_request_complete(stat_request_complete_t *p)
{
  p->max_time = 0;
  p->num = 0;
  p->time = 0;
}

void
stat_init(stat_item_t *item)
{
  item->total_num = 0;
  stat_init_request_complete(&(item->success));
  stat_init_request_complete(&(item->notfound));
}

void
stat_add(stat_request_complete_t *s, uint64_t time)
{
  s->num++;
  s->time+=time;
  s->max_time = time > s->max_time ? time : s->max_time;
}
