#ifndef READ_MEM_H
#define READ_MEM_H

#include "reader.h"

page_t * mem_get(request_t *);
page_t * mem_set(request_t *, page_t *);
page_t * mem_del(request_t *);

#endif // READ_MEM_H
