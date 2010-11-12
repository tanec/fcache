#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "read_mem.h"
#include "CMiniCache.h"
#include "smalloc.h"
#include "settings.h"
#include "md5.h"
#include "util.h"

static CMiniCache *cache;

void
mem_release(page_t *page)
{
  if (page != NULL) {
    sfree(page);
    SYNC_SUB(&total_pages, 1);
  }
}

static int
cmfree(void *ptr)
{
  mem_release(ptr);
  return 0;
}

void
mem_init()
{
  size_t hashSize    = cfg.maxpage * 3;
  size_t maxDataSize = cfg.maxmem - 8*1024*1024;
  size_t maxDataNum  = cfg.maxpage;
  size_t oneAllocNum = 8000;
  cache = CMiniCache_alloc(hashSize, maxDataSize, maxDataNum, oneAllocNum, cmfree);
  if (cache == NULL) {
    perror("CMiniCache_alloc failed");
    exit(EXIT_FAILURE);
  }
}

page_t *
mem_get(md5_digest_t *md5)
{
  page_t *ret = NULL;
  size_t sz = 0;
  GetData(cache, md5, (void *)&ret, &sz);
  return ret;
}

void
mem_set(md5_digest_t *md5, page_t *page)
{
  if (page == NULL) return;

  AddData(cache, md5, page, page->page_len);
}

void
mem_del(map_key_t key)
{
  DelData(cache, key);
}

void
mem_export(char *dest, size_t len)
{
  exportStats(cache, dest, len);
}
