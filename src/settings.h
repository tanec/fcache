#ifndef CONFIG_H
#define CONFIG_H

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
  int num_threads;
  int maxconns;

  //read from file
  char *base_dir;
} setting_t;

static setting_t cfg = {
  0,

  tcp, "127.0.0.1", 2012,
  "/tmp/fcache.socket",

  "/var/run/fcache.pid",
  16,
  4096
};

void read_cfg(setting_t *, char *);

#endif // CONFIG_H
