#include <stdbool.h>
#include <stdarg.h>
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
#include "log.h"

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

md5_digest_t *
md5_dir(request_t *req)
{
  if (req->dig_dir==NULL) {
    req->dig_dir = &(req->digests[0]);

    size_t len1 = strlen(req->host);
    size_t len2 = strlen(req->keyword);
    char d[len1+len2+1];
    memcpy(d, req->host, len1);
    memcpy(d+len1, req->keyword, len2);
    d[len1+len2] = '\0';

    tlog(DEBUG, "md5_dir(%s)", d);
    md5_digest(d, len1+len2, req->dig_dir->digest);
  }
  return req->dig_dir;
}

md5_digest_t *
md5_file(request_t *req)
{
  if (req->dig_file==NULL) {
    req->dig_file = &(req->digests[1]);

    tlog(DEBUG, "md5_file(%s)", req->url);
    md5_digest(req->url, strlen(req->url), req->dig_file->digest);
  }
  return req->dig_file;
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
request_init(request_t *req)
{
  req->host     = NULL;
  req->keyword  = NULL;
  req->url      = NULL;

  req->sticky   = false;

  req->dig_dir  = NULL;
  req->dig_file = NULL;

  memset(req->data, 0, REQDATALEN);
  req->pos      = 0;
}

const char *
request_store(request_t *req, int va_num, ...)
{
  char *ret = req->data+req->pos, *s;
  va_list arg;
  int i;

  va_start(arg, va_num);
  for (i=0; i<va_num; i++) {
    s = va_arg(arg, char *);
    if (s!=NULL) {
      size_t len = strlen(s);
      if (req->pos+len<REQDATALEN) strcat(ret, s);
    }
  }
  va_end(arg);

  req->pos+=strlen(ret)+1;
  return ret;
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

  page = mem_get(md5_file(req));

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

  page = file_get(md5_dir(req), md5_file(req));

  if (page != NULL) {
    if (req->sticky) page->level = -18;
    stat_add(&(item.success), current_time_millis() - s);
  } else {
    stat_add(&(item.notfound), current_time_millis() - s);
  }
  return page;
}

page_t *
process_get(request_t *req)
{
  page_t *page = NULL;
  int curr_stat = current_stat_slot();

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
process_auth(char *igid, page_t *page)
{
  if (page == NULL) return NULL;

  page_t * ret = page;
  uint64_t s = current_time_millis();
  stat_item_t item = statics[current_stat_slot()].auth;
  item.total_num++;

  if (!auth_http(igid, page->head.keyword, page->head.auth_type, page->head.param))
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
    sfree(mem_set(req->dig_file, page)); // save in memory if expired?
    mem_lru();
    if (expire) {
      udp_notify_expire(req, page); //udp: notify
    }
  } else if (page->from == MEMORY) {
    if (expire) {
      page_t *p1 = file_get(md5_dir(req), md5_file(req));
      if (p1 == NULL) { // not in fs: delete
        sfree(mem_del(req->dig_file));
      } else if (is_expire(p1)) { // expire in fs
        udp_notify_expire(req, p1); // udp: notify
        sfree(p1);
      } else { // valid on fs
        sfree(mem_set(req->dig_file, p1));
        mem_lru(); // necessary?
      }
    } else {
      mem_access(page);
    }
  }
}
