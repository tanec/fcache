#ifndef ZHONGSOU_KEYWORD_H
#define ZHONGSOU_KEYWORD_H

#include <sys/types.h>

typedef struct {
  char *key;
  char *value;
} str_map_node_t;

typedef struct {
  size_t len;
  str_map_node_t *base;
} str_map_t;

void read_domain(const char *);
void read_synonyms(const char *);

char * domain2kw(char *);
char * synonyms2kw(char *);

#endif // ZHONGSOU_KEYWORD_H
