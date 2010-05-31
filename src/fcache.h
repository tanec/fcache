#ifndef FCACHE_H
#define FCACHE_H

#include <sys/types.h>
#include <stdint.h>
#include <sys/un.h>
#include <netinet/in.h>

#define EX_OSERR 3
#define REQUESTS_MAX __SHRT_MAX__+__SHRT_MAX__

#define ROLE_RESPONDER  1
#define ROLE_AUTHORIZER 2
#define ROLE_FILTER     3

enum {
  PROTOST_REQUEST_COMPLETE = 0,
  PROTOST_CANT_MPX_CONN    = 1,
  PROTOST_OVERLOADED       = 2,
  PROTOST_UNKNOWN_ROLE     = 3
};

typedef struct {
  uint16_t id;
  uint16_t role;
  uint8_t keepconn;
  uint8_t stdin_eof;
  uint8_t terminate;
  uint8_t padding;
  struct bufferevent *bev;
} fcgi_request_t;

fcgi_request_t *_requests[REQUESTS_MAX];

#endif // FCACHE_H
