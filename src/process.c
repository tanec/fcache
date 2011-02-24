#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "smalloc.h"
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
  uint64_t udp_send, udp_recv, udp_hit;
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
    statics[curr].udp_send = 0;
    statics[curr].udp_recv = 0;
    statics[curr].udp_hit  = 0;
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
  req->igid     = NULL;

  req->force_refresh = false;

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
process_expire(md5_digest_t *dig)
{
  tlog(DEBUG, "expire: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
       dig->digest[0], dig->digest[1], dig->digest[2], dig->digest[3],
       dig->digest[4], dig->digest[5], dig->digest[6], dig->digest[7],
       dig->digest[8], dig->digest[9], dig->digest[10], dig->digest[11],
       dig->digest[12], dig->digest[13], dig->digest[14], dig->digest[15]);
  int slot = current_stat_slot();
  statics[slot].udp_recv ++;
  page_t *page = mem_get(dig);
  if (page != NULL) {
    statics[slot].udp_hit++;
    mem_del(dig);
  }
}

void
process_init()
{
  mem_init();
  memset(&statics, 0, sizeof(statics));

  udp_listen_expire(process_expire);
}

void
process_sticky()
{//load sticky page
  if (cfg.sticky_url_file != NULL) {
    page_t *page;
    FILE *file;
    int len = 512, i;
    char line[len], domain[len], keyword[len], *l;
    md5_digest_t md5d, md5f;

    file = fopen(cfg.sticky_url_file, "r");
    if (file != NULL) {
      do {
        l=fgets(line, len, file);
        if (l==NULL) break;

        for(i=0; i<strlen(l); i++) {
          keyword[i] = *(l+i);
          switch(keyword[i]) {
          case ' ':
          case '\t':
            keyword[i] = '\0';
          }

          if (keyword[i] == '\0') break;
        }

        l = strstr(line, "://");
        if (strncmp(l, "://", 3)==0) l+=3;

        for(i=0; i<strlen(l); i++) {
          domain[i] = *(l+i);
          if (domain[i] == '\0') break;
        }

        strcat(domain, keyword);
        md5_digest(domain, strlen(domain), md5d.digest);
        md5_digest(l, strlen(l), md5f.digest);
        page = file_get(&md5d, &md5f);
        if (page != NULL) {
          mem_set(&md5f, page);
        }
      } while(1);

      fclose(file);
    }
  }
}

page_t *
process_mem(request_t *req, int curr_stat)
{
  page_t *page;
  uint64_t s = current_time_micros();
  stat_item_t *item = &statics[curr_stat].mem;
  item->total_num++;

  page = mem_get(md5_file(req));
  if (page != NULL) {
    if(strcmp(req->keyword, page->head.keyword)!=0)
      page = NULL;
  }

  if (page != NULL) {
    stat_add(&(item->success), current_time_micros() - s);
  } else {
    stat_add(&(item->notfound), current_time_micros() - s);
  }
  return page;
}

page_t *
process_fs(request_t *req, int curr_stat)
{
  page_t *page;
  uint64_t s = current_time_millis();
  stat_item_t *item = &statics[curr_stat].fs;
  item->total_num++;

  page = file_get(md5_dir(req), md5_file(req));

  if (page != NULL) {
    stat_add(&(item->success), current_time_millis() - s);
  } else {
    stat_add(&(item->notfound), current_time_millis() - s);
  }
  return page;
}

static inline page_t *
dup_page(page_t *page)
{
  if (page == NULL) return NULL;
  page_t *np = smalloc(page->page_len);
  memcpy(np, page, page->page_len);

  np->body = (char *)np+sizeof(page_t);
  char *strpos = (char*)np->body + np->body_len;

  page_head_t *head = &np->head;

  head->keyword = strpos;
  strpos += strlen(head->keyword)+1;
  head->ig = strpos;
  strpos += strlen(head->ig)+1;
  head->param = strpos;

  SYNC_ADD(&total_pages, 1);
  return np;
}

page_t *
process_get(request_t *req)
{
  page_t *page = NULL;
  int curr_stat = current_stat_slot();

  page = process_mem(req, curr_stat);
  if (page != NULL) {
    return dup_page(page);
  }

  if (!cfg.base_dir_ok) return NULL;

  page = process_fs(req, curr_stat);
  if (page != NULL) {
    mem_set(req->dig_file, page);
    return dup_page(page);
  }

  return NULL;
}

bool
process_auth(const char *igid, page_t *page)
{
  if (page == NULL) return false;

  bool ret = false;
  uint64_t s = current_time_millis();
  stat_item_t *item = &statics[current_stat_slot()].auth;
  item->total_num++;

  ret = auth_http(igid, page->head.keyword, page->head.auth_type, page->head.param);

  if (ret != false) {
    stat_add(&(item->success), current_time_millis() - s);
  } else {
    stat_add(&(item->notfound), current_time_millis() - s);
  }
  return ret;
}

int
process_upstream_start(void)
{
  int curr;

  curr = current_stat_slot();
  statics[curr].net.total_num++;

  return curr;
}

void
process_upstream_end(int slot, uint64_t start, bool success)
{
  stat_item_t *item = &statics[slot].net;
  if (success) {
    stat_add(&(item->success), current_time_millis() - start);
  } else {
    stat_add(&(item->notfound), current_time_millis() - start);
  }
}

void
process_cache(request_t *req, page_t *page)
{// send done
  if (page == NULL) return;

  bool expire = is_expire(page);
  if (expire) {
    udp_notify_expire(req, page);
    int slot = current_stat_slot();
    statics[slot].udp_send ++;
  }

  mem_release(page);
}

static inline void
ps_itoa(char *dest, const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  vsprintf(dest, fmt, args);
  va_end(args);
}

static inline void
process_stat_item_html(char *html, stat_item_t *item)
{
  char line[512];
  strcat(html, "<td><pre>");
  ps_itoa(line, "total=%llu\n", item->total_num);
  strcat(html, line);
  ps_itoa(line, "success=(num=%llu, time=%llu, max=%llu)\n", item->success.num,  item->success.time,  item->success.max_time);
  strcat(html, line);
  ps_itoa(line, " failed=(num=%llu, time=%llu, max=%llu)",   item->notfound.num, item->notfound.time, item->notfound.max_time);
  strcat(html, line);
  strcat(html, "</pre></td>");
}

void
process_stat_html(char *result)
{
  int i, c, pos;
  char buf[512];

  strcat(result,
         "<html><head>\
         <title>fcache in-memory statistics</title>\
         <style>\n\
         table  {border: 3px solid  #333; width: 100%;}\n\
         tr+tr  {border-top: 2px dotted #333;}\n\
         th, td {border: 1px dotted gray;}\n\
         th {text-align: left;} \n\
         </style></head><body>");
  sprintf(buf,"%lu page use %lu bytes &nbsp; -- &nbsp;", total_pages, smalloc_used_memory());
  strcat(result, buf);
  mem_export(buf, 511);
  strcat(result, buf);
  c = current_stat_slot();

  strcat(result, "<table><tr>\
         <th>slot</th>\
         <th>memory(us)</th>\
         <th>file system(ms)</th>\
         <th>authorication(ms)</th>\
         <th>upstream(ms)</th>\
         <th>udp</th>\
         </tr>");
  for (i=0; i<STAT_HOURS; i++) {
    pos = (STAT_HOURS+c-i)%STAT_HOURS;
    stat_t *st = &statics[pos];
    strcat(result, "<tr>");
    sprintf(buf, "<td><pre>cur=%d\npos=%d</pre></td>", c, pos);
    strcat(result, buf);
    process_stat_item_html(result, &st->mem);
    process_stat_item_html(result, &st->fs);
    process_stat_item_html(result, &st->auth);
    process_stat_item_html(result, &st->net);
    sprintf(buf, "<td><pre>send=%llu\nrecv=%llu/%llu</pre></td>",
	    st->udp_send, st->udp_recv, st->udp_hit);
    strcat(result, buf);
    strcat(result, "</tr>");
  }
  strcat(result, "</table><br/><table><tr><th>type</th><th>url</th><th>host</th><th>port</th><th>status</th></tr>");
  snprintf(buf, 511, "<tr><td>%s</td><td>%s</td><td></td><td></td><td>%s</td></tr>",
	   "mfs", cfg.base_dir, cfg.base_dir_ok?"ok":"down");
  strcat(result, buf);
  snprintf(buf, 511, "<tr><td>pass</td><td></td><td>127.0.0.1</td><td></td><td>%x</td></tr>",
	   cfg.pass);
  strcat(result, buf);
  server_group_t *grps[] = {&cfg.http, &cfg.owner, &cfg.udp_notify};
  int ii, j, n=sizeof(grps)/sizeof(server_group_t *);
  for (ii=0; ii<n; ii++) {
    server_group_t *g = grps[ii];
    for (j=0; j<g->num; j++) {
      snprintf(buf, 511, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%d</td><td>%s</td></tr>",
	       g->servers[j].type, g->servers[j].url, g->servers[j].host, g->servers[j].port, g->servers[j].up?"ok":"down");
      strcat(result, buf);
    }
  }
  strcat(result, "</table></body></html>");
}
