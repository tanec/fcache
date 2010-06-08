#ifndef CONFIG_H
#define CONFIG_H

#include <sys/types.h>
#include <stdint.h>
#include "log.h"

typedef enum {
  tcp,
  udp,
  domain_socket
} conn_t;

typedef struct {
  char *url;
  char *host;
  uint16_t port;
} server_t;

typedef struct {
  server_t *servers;
  uint8_t num, idx;
} server_group_t;

typedef struct {
  int daemon;
  char *log_file;
  log_level_t log_level;

  conn_t conn_type;
  char *bind_addr;
  uint16_t port;
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

  //udp notify other server
  server_group_t udp_notify;
  //udp notify me
  server_t udp_server;

  //http auth
  server_group_t auth;

  //http upstream;
  server_group_t http;
} setting_t;

extern setting_t cfg;

void init_cfg(void);
void read_cfg(char *);

server_t * next_server_in_group(server_group_t *);

#endif // CONFIG_H
