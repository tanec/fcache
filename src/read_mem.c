#include <stdio.h>
#include "read_mem.h"
#include "skiplist.h"

static map_t *cache;

void
mem_init()
{
cache = map_alloc(&MAP_IMPL_SL, NULL);
}

page_t *
mem_get(request_t *req)
{

    return NULL;
}

page_t *
mem_set(request_t *req, page_t *page)
{
}

page_t *
mem_del(request_t *req)
{

    return NULL;
}
