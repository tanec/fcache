#ifndef CONFIG_H
#define CONFIG_H

#include <sys/types.h>
#include <stdint.h>

typedef enum {
  tcp,
  udp,
  domain_socket
} conn_t;

typedef struct {
  int daemon;

  conn_t conn_type;
  char *bind_addr;
  int port;
  char *socketpath;
  char *status_path;

  char *pid_file;
  int num_threads;
  size_t maxmem;
  size_t min_reserve; // to start lru
  size_t max_reserve; // to stop  lru
  int maxconns;

  //read from file
  char *base_dir;
  char *doamin_file;
  char *synonyms_file;

  //udp notify
  char *udp_notify_host;
  uint16_t udp_notify_port;
} setting_t;

static setting_t cfg;

void init_cfg(void);
void read_cfg(setting_t *, char *);

#endif // CONFIG_H