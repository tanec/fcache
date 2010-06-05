#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "process.h"
#include "reader.h"
#include "read_mem.h"
#include "read_file.h"
#include "md5.h"
#include "util.h"
#include "statistics.h"
#include "settings.h"
#include "zhongsou_net_auth.h"
#include "zhongsou_net_udp.h"

#define STAT_HOURS 24

typedef struct {
  stat_item_t mem;
  stat_item_t fs;
  stat_item_t auth;
  stat_item_t net;
} stat_t;

static int curr=-1;
static stat_t statics[STAT_HOURS];

int
current_stat_slot()
{
  int temp = time(NULL)/3600%STAT_HOURS;
  if (temp != curr) {
    curr = temp;
    //init
    stat_init(&(statics[curr].mem));
    stat_init(&(statics[curr].fs));
    stat_init(&(statics[curr].auth));
    stat_init(&(statics[curr].net));
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

  udp_listen_expire();
}

page_t *
process_mem(request_t *req, int curr_stat)
{
  page_t *page;
  uint64_t s = current_time_millis();
  stat_item_t item = statics[curr_stat].mem;
  item.total_num++;

  page = mem_get(req);

  if (page != NULL) {
    stat_add(&(item.success), current_time_millis() - s);
  } else {
    stat_add(&(item.notfound), current_time_millis() - s);
  }
  return page;
}

page_t *
process_fs(request_t *req, int curr_stat)
{
  page_t *page;
  uint64_t s = current_time_millis();
  stat_item_t item = statics[curr_stat].fs;
  item.total_num++;

  page = file_get(req);

  if (page != NULL) {
    if (req->sticky) page->level = -18;
    stat_add(&(item.success), current_time_millis() - s);
  } else {
    stat_add(&(item.notfound), current_time_millis() - s);
  }
  return page;
}

void
send_status(request_t *req)
{
#define STATUS_LEN 819201
  char buf[STATUS_LEN];
  int n=0, len=STATUS_LEN-1;
  memset(buf, 0, STATUS_LEN);
  n+=snprintf(buf+n, len-n, "<html><head><title>status</title></head><body>");
  n+=snprintf(buf+n, len-n, "");
  n+=snprintf(buf+n, len-n, "</body></html>");
  //TODO
}

page_t *
process_get(request_t *req)
{
  page_t *page = NULL;
  int curr_stat = current_stat_slot();

  size_t l1=strlen(cfg.status_path), l2=strlen(req->url);
  if (l1<=l2 && strncmp(cfg.status_path, req->url, l1) == 0) {
    send_status(req);
    return;//TODO
  }

  page = process_mem(req, curr_stat);
  if (page != NULL) {
    page->from = MEMORY;
    return page;
  }

  page = process_fs(req, curr_stat);
  if (page != NULL) {
    page->from = FILESYSTEM;
    return page;
  }

  return NULL;
}

page_t *
process_auth(request_t *req, page_t *page)
{
  if (page == NULL) return NULL;

  page_t * ret = page;
  uint64_t s = current_time_millis();
  stat_item_t item = statics[current_stat_slot()].auth;
  item.total_num++;

  char *igid;
  if (!auth_http(igid, req->keyword, page->head.auth_type, page->head.param))
    ret = NULL;

  if (page != NULL) {
    stat_add(&(item.success), current_time_millis() - s);
  } else {
    stat_add(&(item.notfound), current_time_millis() - s);
  }
  return ret;
}

page_t *
process_cache(request_t *req, page_t *page)
{// send done
  if (page == NULL) return NULL;

  bool expire = is_expire(page);
  if (page->from == FILESYSTEM) {
    sfree(mem_set(req, page)); // save in memory if expired?
    mem_lru();
    if (expire) {
      udp_notify_expire(req, page); //udp: notify
    }
  } else if (page->from == MEMORY) {
    if (expire) {
      page_t *p1 = file_get(req);
      if (p1 == NULL) { // not in fs: delete
        sfree(mem_del(&(req->dig_file)));
      } else if (is_expire(p1)) { // expire in fs
        udp_notify_expire(req, p1); // udp: notify
        sfree(p1);
      } else { // valid on fs
        sfree(mem_set(req, p1));
        mem_lru(); // necessary?
      }
    } else {
      mem_access(page);
    }
  }
}
