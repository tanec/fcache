#include "settings.h"

void
init_cfg(setting_t *set)
{
  set->daemon = 0;
  set->log_file = "/var/log/fcache.log";
  set->log_level = INFO;

  set->conn_type = tcp;
  set->bind_addr = "127.0.0.1";
  set->port = 2012;

  set->doamin_file = "/tmp/fcache.socket";
  set->status_path = "/fcache-status";
  set->pid_file = "/var/run/fcache.pid";

  set->num_threads = 16;
  set->maxmem = (size_t)2*1024*1024*1024;
  set->min_reserve = (size_t)800*1024*1024;
  set->max_reserve = set->maxmem;
  set->maxconns = 1024;

  set->base_dir = "/tmp";

  set->udp_notify_host = "127.0.0.1";
  set->udp_notify_port = 2013;
}

void
read_cfg(setting_t *cfg, char *file)
{

}
