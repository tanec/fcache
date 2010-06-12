#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>
#include "md5.h"
#include "reader.h"

typedef struct {
  const char *domain;
  char *keyword;
  char *url;

  bool sticky; // lru or not

  uint8_t set_mask; // 0x1: dir md5 set; 0x2: file md5 set
  md5_digest_t dig_dir;
  md5_digest_t dig_file;

  char *enc;
} request_t;

void md5_dir(request_t *);
void md5_file(request_t *);

void process_init();
page_t * process_get(request_t *);
page_t * process_auth(char *, page_t *);
page_t * process_cache(request_t *, page_t *);

#endif // PROCESS_H
