#include <stdio.h>
#include <string.h>
#include "read_mem.h"
#include "skiplist.h"
#include "smalloc.h"
#include "settings.h"
#include "md5.h"

static map_t *cache = NULL;
static time_t total=0, max=0;
static size_t count=0;

int
md5_digest_compare(void *m1, void *m2)
{
  return md5_compare(((map_key_t)m1)->digest, ((map_key_t)m2)->digest);
}
void *
md5_digest_clone(void *orig)
{
  return orig;
}
uint32_t
md5_digest_hash(void *orig)
{
  map_key_t key = (map_key_t)orig;
  return (uint32_t)(key->digest[3] << 24 +
		    key->digest[2] << 16 +
		    key->digest[1] << 8 +
		    key->digest[0]);
}


static datatype_t mem_keytype = {
  md5_digest_compare,
  md5_digest_hash,
  md5_digest_clone
};

void
mem_init()
{
  if (cache != NULL) map_free(cache);

  cache = map_alloc(&MAP_IMPL_SL, &mem_keytype);
  total = 0;
  max = 0;
  count = 0;
}

page_t *
mem_get(request_t *req)
{
  md5_file(req);
  return map_get(cache, &(req->dig_file));
}

page_t *
mem_set(request_t *req, page_t *page)
{
  page_t *ret;
  if (page == NULL) return NULL;

  md5_file(req);
  page->level = max++;
  total += page->level;

  memcpy(&(page->digest), &(req->dig_file), sizeof(md5_digest_t));
  ret = map_set(cache, &(page->digest), page);
  if (ret != NULL)
    total -= ret->level; // replace
  else
    count++; //add
  return ret;
}

page_t *
mem_del(map_key_t key)
{
  page_t *ret;

  ret = map_remove(cache, key);
  if (ret != NULL) {
    count --;
    total -= ret->level;
  }
  return ret;
}

void
mem_access(page_t *page)
{
  if (page == NULL) return;
  total -= page->level;
  page->level = max++;
  total += page->level;
}

void
mem_lru(void)
{
  if (smalloc_used_memory()+cfg.min_reserve < cfg.maxmem) {
    size_t avg = total / (count>0?count:1);
    map_iter_t *iter = map_iter_begin(cache, DOES_NOT_EXIST);
    map_key_t key;
    page_t *page;

    if (iter != NULL)
      while(cfg.maxmem - smalloc_used_memory() < cfg.max_reserve) {
	page = (page_t*)map_iter_next(iter, &key);
	if (page!=NULL && 0<=page->level && page->level<avg) {
          sfree(mem_del(key));
	}
      }
    map_iter_free(iter);
  }
}
