#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "zhongsou_monitor.h"
#include "settings.h"
#include "http-api.h"
#include "thread.h"

//{"name":"202.108.4.67:6668","type":"cms","status":"OK","changeTime":"2010-08-24 17:47:38","sticky":false}
static inline void
do_check_server(server_t *srv, char *url)
{
  http_response_t resp = {NULL, 0};
  if (http_get(url, &resp) && resp.size>5) {
    if (strstr(resp.reply, "\"status\":\"OK\"") != NULL) srv->up=true;
    if (strstr(resp.reply, "\"status\":\"DOWN\"") != NULL) srv->up=false;
  }
  if (resp.reply) free(resp.reply);
}

static inline void
check_server(server_t *s, const char *mon)
{
#define UL 2048
  char dest[UL];

  if (!s) return; 
  if (s->url) {
    snprintf(dest, UL, "%s?type=%s&resource=%s", mon, s->type, s->url);
    do_check_server(s, dest);
  } else if (s->host && s->port!=0) {
    snprintf(dest, UL, "%s?type=%s&resource=%s:%d", mon, s->type, s->host, s->port);
    do_check_server(s, dest);
  }

#undef UL
}

void *
check_servers(void *data)
{
  int i, j, n;
  const char *mon;
  server_t mfs;
  server_group_t *g, *grps[] = {&cfg.http, &cfg.udp_notify};

  mfs.type = "mfs";
  mfs.url  = cfg.base_dir;
  mfs.host = NULL;
  mfs.port = 0;
  mfs.up   = true;

  n=sizeof(grps)/sizeof(server_group_t *);
  mon = cfg.monitor_server.url;
  while(mon != NULL) {
    //check mfs
    check_server(&mfs, mon);
    cfg.base_dir_ok = mfs.up;

    //check other servers
    for(i=0; i<n; i++) {
      g = grps[i];
      for (j=0; j<g->num; j++) {
	check_server(&g->servers[j], mon);
      }
    }
    sleep(2);
  }
  return NULL;
}

void
poll_monitor_result(void)
{
  if(cfg.monitor_server.url)
    create_worker(check_servers, NULL);
}
