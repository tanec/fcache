#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>
#include "md5.h"

typedef struct {
  char *domain;
  char *keyword;
  char *url;

  bool sticky; // lru or not

  uint8_t set_mask; // 0x1: dir md5 set; 0x2: file md5 set
  md5_digest_t dig_dir;
  md5_digest_t dig_file;

  char *enc;
} request_t;

typedef struct {

} response_t;

void md5_dir(request_t *);
void md5_file(request_t *);

void process_init();
void process(request_t *, response_t *);

#endif // PROCESS_H
