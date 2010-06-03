#include <string.h>
#include "settings.h"
#include "util.h"

setting_t cfg = {};

void
init_cfg(void)
{
  cfg.daemon = 0;
  cfg.log_file = "/var/log/fcache.log";
  cfg.log_level = DEBUG;

  cfg.conn_type = tcp;
  cfg.bind_addr = "127.0.0.1";
  cfg.port = 2012;

  cfg.doamin_file = NULL;
  cfg.synonyms_file= NULL;

  cfg.status_path = "/fcache-status";
  cfg.pid_file = "/var/run/fcache.pid";

  cfg.num_threads = 16;
  cfg.maxmem = (size_t)2*1024*1024*1024;
  cfg.min_reserve = (size_t)800*1024*1024;
  cfg.max_reserve = cfg.maxmem;
  cfg.maxconns = 1024;

  cfg.base_dir = "/tmp";

  cfg.udp_notify_host = "127.0.0.1";
  cfg.udp_notify_port = 2013;
}

void
cfg_set(const char *k, char *v)
{
  tlog(DEBUG, "cfg_set(%s, %s)", k, v);
#define cfg_set_m(kk, vv) if (strcmp(k, #kk)==0) { cfg.kk=vv; return; }
  cfg_set_m(daemon, atoi(v));
  cfg_set_m(log_file, v);
  cfg_set_m(log_level, atoi(v));
  cfg_set_m(conn_type, atoi(v));
  cfg_set_m(bind_addr, v);
  cfg_set_m(port, atoi(v));
  cfg_set_m(socketpath, v);
  cfg_set_m(status_path, v);
  cfg_set_m(pid_file, v);
  cfg_set_m(num_threads, atoi(v));
  cfg_set_m(maxmem, atoi(v));
  cfg_set_m(min_reserve, atoi(v));
  cfg_set_m(max_reserve, atoi(v));
  cfg_set_m(maxconns, atoi(v));
  cfg_set_m(base_dir, v);
  cfg_set_m(doamin_file, v);
  cfg_set_m(synonyms_file, v);
  cfg_set_m(udp_notify_host, v);
  cfg_set_m(udp_notify_port, atoi(v));
#undef cfg_set_m
}

void
read_cfg(char *file)
{
  tlog(DEBUG, "read_cfg(%s)", file);
  mmap_array_t mt;
  if (mmap_read(&mt, file)) {
    int i;
    char data[mt.len+1], *k=NULL, *v=NULL;

    memset(data, '\n', mt.len+1);
    memcpy(data, mt.data, mt.len);

    for (i=0; i<mt.len+1; i++) {
      switch(data[i]) {
      case '\n':
        data[i] = '\0';
        if (k != NULL && v != NULL) {
          cfg_set(k, v);
        }
        k = NULL;
        v = NULL;
        break;
      case '\r':
      case '\t':
      case ' ':
      case '=':
        data[i] = '\0';
        break;
      default:
        if (k == NULL) {
          k = data+i;
        } else if (v == NULL && data[i-1]=='\0') {
          v = data+i;
        }
        break;
      }
    }
    mmap_close(&mt);
  }
}
