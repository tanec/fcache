#include <string.h>
#include <stdlib.h>
#include <libconfig.h>

#include "settings.h"
#include "util.h"
#include "log.h"

setting_t cfg = {};

static inline void
init_server_group(server_group_t *g)
{
  g->idx=0;
  g->num=0;
  g->servers=NULL;
}

static inline void
set_log_cfg(void)
{
  default_level = cfg.log_level;
  log_file = (char*)cfg.log_file;
}

void
init_cfg(void)
{
  cfg.daemon = 0;
  cfg.log_file = NULL;
  cfg.log_level = DEBUG;

  cfg.conn_type = tcp;
  cfg.bind_addr = "127.0.0.1";
  cfg.port = 2046;

  cfg.page_encoding = "UTF-8";

  cfg.doamin_file = NULL;
  cfg.synonyms_file= NULL;

  cfg.status_path = "/fcache/status";
  cfg.monitor_path = "/fcache/mon";
  cfg.read_kw_path = "/fcache/readkw";
  cfg.pid_file = "/var/run/fcache.pid";

  cfg.num_threads = 16;
  cfg.maxmem = (uint64_t)2*1024*1024*1024;
  cfg.min_reserve = (uint64_t)800*1024*1024;
  cfg.max_reserve = cfg.maxmem;
  cfg.maxconns = 1024;

  cfg.base_dir = "/tmp";

  //notify others
  init_server_group(&cfg.udp_notify);
  //notify me
  cfg.udp_server.host = "127.0.0.1";
  cfg.udp_server.port = 2046;
  //auth
  init_server_group(&cfg.auth);
  //http
  init_server_group(&cfg.http);
  cfg.multi_keyword_domains = NULL;
  cfg.multi_keyword_domains_len = 0;

  set_log_cfg();
}

static void inline
read_server_group(config_t *c, const char *path, server_group_t *servers)
{
  int i;
  config_setting_t *set, *set1;
  set = config_lookup(c, path);
  if (set != NULL) {
    int len = config_setting_length(set);
    if (len > 0) {
      servers->idx=0;
      servers->num=len;
      servers->servers = calloc(len, sizeof(server_t));
      for(i=0; i<len;i++) {
        set1 = config_setting_get_elem(set,i);
        if (set1 == NULL) {
          perror("wrong server setting");
          exit(EXIT_FAILURE);
        }
        if(config_setting_lookup_string(set1, "url",  &servers->servers[i].url))  tlog(DEBUG, "%d, %s.url=%s", i, path, servers->servers[i].url);
        if(config_setting_lookup_string(set1, "host", &servers->servers[i].host)) tlog(DEBUG, "%d, %s.host=%s",i, path, servers->servers[i].host);
        if(config_setting_lookup_int   (set1, "port", &servers->servers[i].port)) tlog(DEBUG, "%d, %s.port=%d",i, path, servers->servers[i].port);
      }
    }
  }
}

void
read_cfg(char *file)
{
  tlog(DEBUG, "read_cfg(%s)", file);
  static config_t c; // hold it

  config_init(&c);
  if(! config_read_file(&c, file)) {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(&c),
            config_error_line(&c), config_error_text(&c));
    config_destroy(&c);
  } else {
    int r, n;
    r=config_lookup_int   (&c, "daemon",         &cfg.daemon);        tlog(DEBUG, "%d, cfg.daemon=%d",        r,cfg.daemon);
    r=config_lookup_string(&c, "log.file",       &cfg.log_file);      tlog(DEBUG, "%d, cfg.log_file=%s",      r,cfg.log_file);
    r=config_lookup_int   (&c, "log.level",      &cfg.log_level);     tlog(DEBUG, "%d, cfg.log_level=%d",     r,cfg.log_level);
    r=config_lookup_int   (&c, "conn.type",      &cfg.conn_type);     tlog(DEBUG, "%d, cfg.conn_type=%d",     r,cfg.conn_type);
    r=config_lookup_int   (&c, "conn.max",       &cfg.maxconns);      tlog(DEBUG, "%d, cfg.maxconns=%d",      r,cfg.maxconns);
    r=config_lookup_string(&c, "conn.host",      &cfg.bind_addr);     tlog(DEBUG, "%d, cfg.bind_addr=%s",     r,cfg.bind_addr);
    r=config_lookup_int   (&c, "conn.port",      &cfg.port);          tlog(DEBUG, "%d, cfg.port=%d",          r,cfg.port);
    r=config_lookup_string(&c, "conn.pipe",      &cfg.socketpath);    tlog(DEBUG, "%d, cfg.socketpath=%s",    r,cfg.socketpath);
    r=config_lookup_string(&c, "run.status",     &cfg.status_path);   tlog(DEBUG, "%d, cfg.status_path=%s",   r,cfg.status_path);
    r=config_lookup_string(&c, "run.monitor",    &cfg.monitor_path);  tlog(DEBUG, "%d, cfg.monitor_path=%s",  r,cfg.monitor_path);
    r=config_lookup_string(&c, "run.readkw",     &cfg.read_kw_path);  tlog(DEBUG, "%d, cfg.read_kw_path=%s",  r,cfg.read_kw_path);
    r=config_lookup_string(&c, "run.pidfile",    &cfg.pid_file);      tlog(DEBUG, "%d, cfg.pid_file=%s",      r,cfg.pid_file);
    r=config_lookup_int   (&c, "thread.num",     &cfg.num_threads);   tlog(DEBUG, "%d, cfg.num_threads=%d",   r,cfg.num_threads);
    r=config_lookup_int   (&c, "mem.max",        &n);if(r) cfg.maxmem     =(uint64_t)n*1024*1024; tlog(DEBUG, "%d, cfg.maxmem=%llu",      r,cfg.maxmem);
    r=config_lookup_int   (&c, "mem.min_reserve",&n);if(r) cfg.min_reserve=(uint64_t)n*1024*1024; tlog(DEBUG, "%d, cfg.min_reserve=%llu", r,cfg.min_reserve);
    r=config_lookup_int   (&c, "mem.max_reserve",&n);if(r) cfg.max_reserve=(uint64_t)n*1024*1024; tlog(DEBUG, "%d, cfg.max_reserve=%llu", r,cfg.max_reserve);
    r=config_lookup_string(&c, "fs.dir",         &cfg.base_dir);      tlog(DEBUG, "%d, cfg.base_dir=%s",      r,cfg.base_dir);
    r=config_lookup_string(&c, "fs.encoding",    &cfg.page_encoding); tlog(DEBUG, "%d, cfg.page_encoding=%s", r,cfg.page_encoding);
    r=config_lookup_string(&c, "keyword.domain", &cfg.doamin_file);   tlog(DEBUG, "%d, cfg.doamin_file=%s",   r,cfg.doamin_file);
    r=config_lookup_string(&c, "keyword.sino",   &cfg.synonyms_file); tlog(DEBUG, "%d, cfg.synonyms_file=%s", r,cfg.synonyms_file);
    r=config_lookup_string(&c, "keyword.sticky", &cfg.sticky_url_file);tlog(DEBUG,"%d, cfg.sticky_url_file=%s", r,cfg.sticky_url_file);
    r=config_lookup_string(&c, "udp_server.host",&cfg.udp_server.host);tlog(DEBUG,"%d, cfg.udp_server.host=%s", r,cfg.udp_server.host);
    r=config_lookup_int   (&c, "udp_server.port",&cfg.udp_server.port);tlog(DEBUG,"%d, cfg.udp_server.port=%d", r,cfg.udp_server.port);

    read_server_group(&c, "servers.notify", &cfg.udp_notify);
    read_server_group(&c, "servers.auth",   &cfg.auth);
    read_server_group(&c, "servers.http",   &cfg.http);


    config_setting_t *set, *set1;
    set = config_lookup(&c, "multi_keyword_domains");
    if (set != NULL) {
      cfg.multi_keyword_domains_len = config_setting_length(set);
      if (cfg.multi_keyword_domains_len > 0) {
        cfg.multi_keyword_domains = calloc(cfg.multi_keyword_domains_len, sizeof(char *));
        int i;
        const char *s;
        for(i=0; i<cfg.multi_keyword_domains_len;i++) {
          s = config_setting_get_string_elem(set,i);
          if (s == NULL) {
            perror("wrong multi_keyword_domains setting");
            exit(EXIT_FAILURE);
          }
          cfg.multi_keyword_domains[i] = strdup(s);
          tlog(DEBUG, "%d, domain=%s", i, cfg.multi_keyword_domains[i]);
        }
      }
    }

    set_log_cfg();
  }
}

server_t *
next_server_in_group(server_group_t *group)
{
  if (group==NULL || group->num<1 || group->servers==NULL) return NULL;
  if (group->idx<1) group->idx = 0;
  return &group->servers[(group->idx++)%group->num];
}
