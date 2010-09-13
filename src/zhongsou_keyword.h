#ifndef ZHONGSOU_KEYWORD_H
#define ZHONGSOU_KEYWORD_H

#include <sys/types.h>
#include <stdbool.h>

typedef struct {
  char *key;
  char *value;
} str_map_node_t;

typedef struct {
  size_t len;
  str_map_node_t *base;
} str_map_t;

void read_lock_init(void);
void read_domain(const char *);
void read_synonyms(const char *);

bool is_multi_keyword_domain(const char *);
const char * domain2kw(const char *);
const char * synonyms2kw (const char *, const char *);
const char * find_keyword(const char *, const char *);

#endif // ZHONGSOU_KEYWORD_H
