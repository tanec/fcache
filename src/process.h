#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>
#include "md5.h"
#include "reader.h"

#define REQDATALEN 8192

typedef struct {
  const char *host;
  const char *keyword;
  const char *url;
  const char *igid;

  bool force_refresh;
  bool sticky; // lru or not

  md5_digest_t *dig_dir;
  md5_digest_t *dig_file;
  md5_digest_t digests[2];

  char data[REQDATALEN];
  uint16_t pos;
} request_t;

void request_init(request_t *);
const char * request_store(request_t *, int, ...);

md5_digest_t * md5_dir(request_t *);
md5_digest_t * md5_file(request_t *);

void process_init();
page_t * process_get(request_t *);
page_t * process_auth(const char *, page_t *);
page_t * process_cache(request_t *, page_t *);

int  process_upstream_start(void);
void process_upstream_end(int, uint64_t, bool);

void     process_stat_html(char *);

#endif // PROCESS_H
