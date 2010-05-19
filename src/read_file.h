#ifndef READ_FILE_H
#define READ_FILE_H

#include "reader.h"

page_t* file_read(request_t *);
void    file_cache(request_t *, page_t *);

static reader_t read_file = {
  &file_read,
  &file_cache
};

#endif // READ_FILE_H
