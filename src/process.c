#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "process.h"
#include "reader.h"
#include "read_mem.h"
#include "read_file.h"
#include "md5.h"
#include "statistics.h"

#define STAT_HOURS 24

typedef struct {
  stat_item_t all;
  stat_item_t mem;
  stat_item_t fs;
  stat_item_t net;
} stat_t;

typedef enum {
  mem,
  fs
} ds_t;

static int curr=-1;
static stat_t statics[STAT_HOURS];

int
current_stat_slot()
{
  int temp = time(NULL)/3600%STAT_HOURS;
  if (temp != curr) {
    curr = temp;
    //init
  }
  return curr;
}

void
md5_dir(request_t *req)
{
  if ((req->set_mask&0x1) == 0) {
    size_t len1 = strlen(req->domain);
    size_t len2 = strlen(req->keyword);
    char d[len1+len2+1];
    memcpy(d, req->domain, len1);
    memcpy(d+len1, req->keyword, len2);
    d[len1+len2] = 0;
    md5_digest(d, len1+len2, req->dig_dir.digest);
    req->set_mask |= 0x1;
  }
}

void
md5_file(request_t *req)
{
  if ((req->set_mask&0x2) == 0) {
    md5_digest(req->url, strlen(req->url), req->dig_file.digest);
    req->set_mask |= 0x2;
  }
}

bool
is_expire(page_t *page)
{
  if ((page->head).valid) {
    time_t curr = time(NULL);
    time_t expire = (page->head).time_expire/1000;
    return (expire < curr)?true:false;
  }
  return true;
}

void
process_init()
{
  mem_init();
  memset(&statics, 0, sizeof(statics));
}

page_t *
process_mem(request_t *req, response_t *resp, int curr_stat)
{
  page_t *page;
  struct timespec s, e;
  stat_item_t item = statics[curr_stat].mem;
  item.total_num++;

  touch_timespec(&s);
  page = mem_get(req);
  touch_timespec(&e);

  if (page != NULL) {
    stat_add(&(item.success), time_diff(s, e));
  } else {
    stat_add(&(item.notfound), time_diff(s, e));
  }
  return page;
}

page_t *
process_fs(request_t *req, response_t *resp, int curr_stat)
{
  page_t *page;
  struct timespec s, e;
  stat_item_t item = statics[curr_stat].fs;
  item.total_num++;

  touch_timespec(&s);
  page = file_get(req);
  touch_timespec(&e);

  if (page != NULL) {
    if (req->sticky) page->level = -18;
    stat_add(&(item.success), time_diff(s, e));
  } else {
    stat_add(&(item.notfound), time_diff(s, e));
  }
  return page;
}

void
process(request_t *req, response_t *resp)
{
  struct timespec all_enter, all_fin;
  struct timespec net_enter, net_fin;
  uint64_t use_time;
  int curr_stat = current_stat_slot();

  page_t *page;
  ds_t from = mem;

  touch_timespec(&all_enter);
  statics[curr_stat].all.total_num++;

  page = process_mem(req, resp, curr_stat);

  // fs block
  if (page == NULL) {
    page = process_fs(req, resp, curr_stat);
    from = fs;
  }

  /* net
   *  --  success: write 2 client;
   *  -- notfound: request upstream server + write 2 client
   */
  {
    struct timespec s, e;
    stat_item_t item = statics[curr_stat].net;
    item.total_num++;

    touch_timespec(&s);
    if (page == NULL) {
      // pass to upstream servers
    } else {
      //TODO: send to client
      printf("mem=%x\n", smalloc_used_memory());
    }
    touch_timespec(&e);
    use_time = time_diff(s, e);
    if (page != NULL) {
      stat_add(&(item.success), use_time);
    } else {
      stat_add(&(item.notfound), use_time);
    }
  }

  // send done
  if (page != NULL) {
    bool expire = is_expire(page);
    if (from == fs) {
      sfree(mem_set(req, page)); // save in memory if expired?
      mem_lru();
      if (expire) {
	//udp: notify
      }
    } else if (from == mem) {
      if (expire) {
	page_t *p1 = file_get(req);
	if (p1 == NULL) { // not in fs: delete
	  sfree(mem_del(&(req->dig_file)));
	} else if (is_expire(p1)) { // expire in fs
	  sfree(p1);
	  // udp: notify
	} else { // valid on fs
	  sfree(mem_set(req, p1));
          mem_lru(); // necessary?
	}
      } else {
	mem_access(page);
      }
    }
  }

  touch_timespec(&all_fin);
  use_time = time_diff(all_enter, all_fin);
  stat_add(&(statics[curr_stat].all.success), use_time);
}
