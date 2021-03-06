#include <stdlib.h>
#include "tmap.h"

struct map {
  const map_impl_t *impl;
  void *data;
};

struct map_iter {
  const map_impl_t *impl;
  void *state;
};

inline map_t *
map_alloc(const map_impl_t *map_impl, const datatype_t *key_type)
{
  map_t *map = (map_t *)malloc(sizeof(map_t));
  map->impl  = map_impl;
  map->data  = map->impl->alloc(key_type);
  return map;
}

inline void
map_free(map_t *map)
{
  map->impl->free_(map->data);
}

inline void
map_print(map_t *map, int verbose)
{
  map->impl->print(map->data, verbose);
}

inline size_t
map_count(map_t *map)
{
  return map->impl->count(map->data);
}

inline map_val_t
map_get(map_t *map, map_key_t key)
{
  return map->impl->get(map->data, key);
}

inline map_val_t
map_set(map_t *map, map_key_t key, map_val_t new_val)
{
  return map->impl->cas(map->data, key, (map_val_t)CAS_EXPECT_WHATEVER, new_val);
}

inline map_val_t
map_add(map_t *map, map_key_t key, map_val_t new_val)
{
  return map->impl->cas(map->data, key, (map_val_t)CAS_EXPECT_DOES_NOT_EXIST, new_val);
}

inline map_val_t
map_cas(map_t *map, map_key_t key, map_val_t expected_val, map_val_t new_val)
{
  return map->impl->cas(map->data, key, expected_val, new_val);
}

inline map_val_t
map_replace(map_t *map, map_key_t key, map_val_t new_val)
{
  return map->impl->cas(map->data, key, (map_val_t)CAS_EXPECT_EXISTS, new_val);
}

inline map_val_t
map_remove(map_t *map, map_key_t key)
{
  return map->impl->remove(map->data, key);
}

inline map_iter_t *
map_iter_begin(map_t *map, map_key_t key)
{
  map_iter_t *iter = (map_iter_t *)malloc(sizeof(map_iter_t));
  iter->impl  = map->impl;
  iter->state = map->impl->iter_begin(map->data, key);
  return iter;
}

inline map_val_t
map_iter_next(map_iter_t *iter, map_key_t *key_ptr)
{
  return iter->impl->iter_next(iter->state, key_ptr);
}

inline void
map_iter_free(map_iter_t *iter)
{
  iter->impl->iter_free(iter->state);
  free(iter);
}
