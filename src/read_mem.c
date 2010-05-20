#include <stdio.h>
#include "read_mem.h"
#include "skiplist.h"

static map_t *cache;

void
mem_init()
{
    cache = map_alloc(&MAP_IMPL_SL, NULL);
    map_print(cache, 1);
}

page_t *
mem_get(request_t *req)
{
    md5_file(req);
    return map_get(cache, req->dig_file);
}

page_t *
mem_set(request_t *req, page_t *page)
{
    md5_file(req);
    return map_set(cache, req->dig_file, page);
}

page_t *
mem_del(request_t *req)
{
    md5_file(req);
    return map_remove(cache, req->dig_file);
}
