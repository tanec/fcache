#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

struct skiplist_node {
  void *key, *value;
  struct skiplist_node *next;
};
typedef struct skiplist_node skiplist_node_t;

struct skiplist_index {
  struct skiplist_node *node;
  struct skiplist_index *down, *right;
  uint8_t level;
};

struct skiplist {
  size_t count;
  long random_seed;
  struct skiplist_index *head;
  int (*compare)(void*, void*);
};
typedef struct skiplist skiplist_t;

skiplist_t * sl_alloc();
void *sl_get(skiplist_t *sklist, void *key);
void *sl_set(skiplist_t *sklist, void *key, void *value);
void *sl_del(skiplist_t *sklist, void *key);
void sl_free(skiplist_t *sklist);

size_t sl_len(const skiplist_t *sklist);

#endif // SKIPLIST_H
