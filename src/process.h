#ifndef PROCESS_H
#define PROCESS_H

#include "md5.h"

typedef struct {
  char *domain;
  char *keyword;
  char *url;
  uint8_t set_mask; // 0x1: dir md5 set; 0x2: file md5 set
  md5_byte_t dig_dir[16];
  md5_byte_t dig_file[16];
} zhongsou_t;

typedef struct {
  char *enc;
  char *host;
  char *uri;

  zhongsou_t inner;
} request_t;

typedef struct {

} response_t;

void md5_dir(zhongsou_t *);
void md5_file(zhongsou_t *);
void process(request_t *, response_t *);

#endif // PROCESS_H
