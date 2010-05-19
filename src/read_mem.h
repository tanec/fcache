#ifndef READ_MEM_H
#define READ_MEM_H

#include "reader.h"

page_t* mem_read(request_t *);
void    mem_cache(request_t *, page_t *);

static reader_t read_mem = {
  &mem_read,
  &mem_cache
};

#endif // READ_MEM_H
