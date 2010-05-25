#include "settings.h"

void
init_cfg(void)
{
  cfg.daemon = 0;

  cfg.conn_type = tcp;
  cfg.bind_addr = "127.0.0.1";
  cfg.port = 2012;

  cfg.doamin_file = "/tmp/fcache.socket";
  cfg.pid_file = "/var/run/fcache.pid";

  cfg.num_threads = 16;
  cfg.maxmem = (size_t)2*1024*1024*1024;
  cfg.maxconns = 4096;

  cfg.base_dir = "/tmp";
}

void
read_cfg(setting_t *cfg, char *file)
{

}
