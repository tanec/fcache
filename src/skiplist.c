#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "skiplist.h"
#include "smalloc.h"

#define MAX_LEVELS 24

enum unlink {
  FORCE_UNLINK,
  ASSIST_UNLINK,
  DONT_UNLINK
};

typedef struct skiplist_index index_t;

void *base = "base node of skip list";

struct skiplist_iter {
  skiplist_node_t *next;
};

/* node section start */
inline bool
cas_value(skiplist_node_t *node, void *cmp, void *value)
{
  return __sync_bool_compare_and_swap(&(node->value), cmp, value);
}

inline bool
cas_next(skiplist_node_t *node, skiplist_node_t *cmp, skiplist_node_t *next)
{
  return __sync_bool_compare_and_swap(&(node->next), cmp, next);
}

inline bool
is_marker(const skiplist_node_t *node)
{
  return node == node->value;
}

inline bool
is_base(const skiplist_node_t *node)
{
  return node->value == base;
}

bool
append_marker(skiplist_node_t *node, skiplist_node_t *next)
{
  skiplist_node_t *marker = smalloc(sizeof(skiplist_node_t));
  marker->key = NULL;
  marker->value = marker;
  marker->next = next;
  bool ret = cas_next(node, next, marker);
  if (!ret)
    sfree(marker);
  return ret;
}

void
help_delete(skiplist_node_t *node, skiplist_node_t *b, skiplist_node_t *f)
// b: predecessor; f: successor
{
  if (f == node->next && node == b->next) {
    if (f == NULL || f->value != f){
      // not already marked
      append_marker(node, f);
    } else {
      if (cas_next(b, node, f->next)) {
	sfree(f);
	sfree(node);
      }
    }
  }
}

void
*valid_value(skiplist_node_t *node)
{
  if (is_marker(node) || is_base(node))
    return NULL;
  return node->value;
}

/* node section end */

/* index section start */
inline bool
cas_right(index_t *index, index_t *cmp, index_t *right)
{
  return __sync_bool_compare_and_swap(&(index->right), cmp, right);
}

inline bool
indexes_deleted_node(index_t *index)
{
  return index->node == NULL || index->node->value == NULL;
}

bool
link(index_t *index, index_t *succ, index_t *new_succ)
{
  if (index->node->value != NULL) {
    new_succ->right = succ;
    return cas_right(index, succ, new_succ);
  }
  return false;
}

bool
unlink(index_t *index, index_t *succ)
{
  if (indexes_deleted_node(index)) {
    return false;
  } else {
    bool ret = cas_right(index, succ, succ->right);
    if (ret)
      sfree(succ);
    return ret;
  }
}

/* index section end */

/* skip list section start */
skiplist_t *
sl_alloc()
{
  skiplist_node_t *bn = smalloc(sizeof(skiplist_node_t));
  bn->key = NULL;
  bn->value = base;

  index_t *head = smalloc(sizeof(index_t));
  head->node = base;
  head->level = 1;
  head->down = NULL;
  head->right = NULL;

  skiplist_t *sklist = smalloc(sizeof(skiplist_t));
  sklist->count = 0;
  sklist->random_seed = random() & 0x0100;
  sklist->compare = NULL;
  sklist->head = head;

  return sklist;
}

void *
sl_get(skiplist_t *sklist, void *key)
{
  
}

void *
sl_set(skiplist_t *sklist, void *key, void *value)
{
  
}

void *
sl_del(skiplist_t *sklist, void *key)
{
  
}

void
sl_free(skiplist_t *sklist)
{
  
}

size_t
sl_len(const skiplist_t *sklist)
{
  return sklist->count;
}

inline bool
cas_head(skiplist_t *sklist, index_t *cmp, index_t *head)
{
  return __sync_bool_compare_and_swap(&(sklist->head), cmp, head);
}

/* ---------------- Comparison utilities -------------- */
int
compare(skiplist_t *sklist, void *k1, void *k2)
{
  if (sklist == NULL || sklist->compare == null)
    return k2 - k1;
  else
    return (*(sklist->compare))(k1, k2);
}

/* ---------------- Traversal -------------- */
/* skip list section end */
