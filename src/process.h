#ifndef PROCESS_H
#define PROCESS_H

#include "md5.h"

typedef struct {
  char *domain;
  char *keyword;
  char *url;
  md5_byte_t dig_dir[16];
  md5_byte_t dig_file[16];
} zhongsou_t;

typedef struct {
  char *enc;
  char *host;
  char *uri;
  char *keyword;
} request_t;

typedef struct {

} response_t;

void process(request_t *, response_t *);

#endif // PROCESS_H
