#include "settings.h"

void
init_cfg(void)
{
  cfg.daemon = 0;

  cfg.conn_type = tcp;
  cfg.bind_addr = "127.0.0.1";
  cfg.port = 2012;

  cfg.doamin_file = "/tmp/fcache.socket";
  cfg.status_path = "/fcache-status";
  cfg.pid_file = "/var/run/fcache.pid";

  cfg.num_threads = 16;
  cfg.maxmem = (size_t)2*1024*1024*1024;
  cfg.min_reserve = (size_t)800*1024*1024;
  cfg.max_reserve = cfg.maxmem;
  cfg.maxconns = 4096;

  cfg.base_dir = "/tmp";

  cfg.udp_notify_host = "127.0.0.1";
  cfg.udp_notify_port = 2013;
}

void
read_cfg(setting_t *cfg, char *file)
{

}
