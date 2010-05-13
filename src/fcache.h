#ifndef FCACHE_H
#define FCACHE_H

#include <sys/types.h>

#define EX_OSERR 3

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

typedef struct {
  u_int64_t request_num;
  u_int64_t pass_num;
  u_int64_t success_num;
  u_int64_t success_time; //total time of success_num
  int max_time; // max time of a single request, success or failed
} stat_summary_t;

#endif // FCACHE_H
