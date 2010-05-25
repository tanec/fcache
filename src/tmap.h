#ifndef TMAP_H
#define TMAP_H

#include <stdint.h>
#include <sys/types.h>
#include "md5.h"
#include "reader.h"

#define DOES_NOT_EXIST 0

// datatype
typedef int      (*cmp_fun_t)   (void *, void *);
typedef void *   (*clone_fun_t) (void *);
typedef uint32_t (*hash_fun_t)  (void *);

typedef struct datatype {
  cmp_fun_t   cmp;
  hash_fun_t  hash;
  clone_fun_t clone;
} datatype_t;

// map
typedef struct map map_t;
typedef struct map_iter map_iter_t;
typedef struct map_impl map_impl_t;

typedef md5_digest_t* map_key_t;
typedef page_t* map_val_t;

map_t *   map_alloc   (const map_impl_t *map_impl, const datatype_t *key_type);
map_val_t map_get     (map_t *map, map_key_t key);
map_val_t map_set     (map_t *map, map_key_t key, map_val_t new_val);
map_val_t map_add     (map_t *map, map_key_t key, map_val_t new_val);
map_val_t map_cas     (map_t *map, map_key_t key, map_val_t expected_val, map_val_t new_val);
map_val_t map_replace (map_t *map, map_key_t key, map_val_t new_val);
map_val_t map_remove  (map_t *map, map_key_t key);
size_t    map_count   (map_t *map);
void      map_print   (map_t *map, int verbose);
void      map_free    (map_t *map);

map_iter_t * map_iter_begin (map_t *map, map_key_t key);
map_val_t    map_iter_next  (map_iter_t *iter, map_key_t *key);
void         map_iter_free  (map_iter_t *iter);

/////////////////////////////////////////////////////////////////////////////////////

#define CAS_EXPECT_DOES_NOT_EXIST ( 0)
#define CAS_EXPECT_EXISTS         (-1)
#define CAS_EXPECT_WHATEVER       (-2)

typedef void *       (*map_alloc_t)  (const datatype_t *);
typedef map_val_t    (*map_cas_t)    (void *, map_key_t , map_val_t, map_val_t);
typedef map_val_t    (*map_get_t)    (void *, map_key_t );
typedef map_val_t    (*map_remove_t) (void *, map_key_t );
typedef size_t       (*map_count_t)  (void *);
typedef void         (*map_print_t)  (void *, int);
typedef void         (*map_free_t)   (void *);

typedef map_iter_t * (*map_iter_begin_t) (void *, map_key_t);
typedef map_val_t    (*map_iter_next_t)  (map_iter_t *, map_key_t *);
typedef void         (*map_iter_free_t)  (map_iter_t *);

struct map_impl {
  map_alloc_t  alloc;
  map_cas_t    cas;
  map_get_t    get;
  map_remove_t remove;
  map_count_t  count;
  map_print_t  print;
  map_free_t   free_;

  map_iter_begin_t iter_begin;
  map_iter_next_t  iter_next;
  map_iter_free_t  iter_free;
};

#endif // TMAP_H
