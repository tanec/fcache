#ifndef CONFIG_H
#define CONFIG_H

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  tcp,
  udp,
  domain_socket
} conn_t;

typedef struct {
  const char *type;
  const char *url;
  const char *host;
  int port;
  bool up;
} server_t;

typedef struct {
  server_t *servers;
  uint8_t num, idx;
} server_group_t;

typedef enum {
  FFF      = 0x0001,
  OWNER    = 0x0002,
  DEAD     = 0x0004,
  SAVE     = 0x0008,
  NOTFOUND = 0x0010
} pass_ctrl_t;

typedef struct {
  int daemon;
  const char *log_file;
  int log_level;

  int conn_type;
  const char *bind_addr;
  int port;
  const char *socketpath;
  const char *status_path;
  const char *monitor_path;
  const char *read_kw_path;
  const char *page_save_path;
  const char *pass_mask_path;
  const char *page403;

  const char *pid_file;
  int num_threads;
  uint64_t maxpage;
  uint64_t maxpagesave;
  uint64_t maxmem;
  uint64_t min_reserve; // to start lru
  uint64_t max_reserve; // to stop  lru
  int maxconns;

  bool base_dir_ok;
  int  pass;
  //read from file
  const char *base_dir;
  const char *page_encoding;
  const char *doamin_file;
  const char *synonyms_file;
  const char *sticky_url_file;

  //get check result
  server_t monitor_server;
  //udp notify other server
  server_group_t udp_notify;
  //udp notify me
  server_t udp_server;
  server_t save_server;

  //http auth
  server_group_t auth;

  //http upstream;
  server_group_t http;
  //http upstream, owner;
  server_group_t owner;

  int multi_keyword_domains_len;
  const char **multi_keyword_domains;
} setting_t;

extern setting_t cfg;

void init_cfg(void);
void read_cfg(char *);

server_t * first_server_in_group(server_group_t *);
server_t * next_server_in_group(server_group_t *);
bool       is_server_available(server_group_t *);

#endif // CONFIG_H
