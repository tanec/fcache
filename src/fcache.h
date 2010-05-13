#ifndef FCACHE_H
#define FCACHE_H

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

  char *pid_file;
  char *cache_file;
  int num_threads;
  int maxconns;
} setting_t;

#endif // FCACHE_H
