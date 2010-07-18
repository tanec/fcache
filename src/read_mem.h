#ifndef READ_MEM_H
#define READ_MEM_H

#include "reader.h"
#include "tmap.h"

page_t * mem_get(md5_digest_t *);
void     mem_release(page_t *);
void mem_set(md5_digest_t *, page_t *);
void mem_del(map_key_t);

void mem_access(page_t *);
void mem_lru(void);

#endif // READ_MEM_H
