#ifndef READ_FILE_H
#define READ_FILE_H

#include "reader.h"
#include "process.h"

page_t* file_get(request_t *);
page_t* file_read_path(char *path);

#endif // READ_FILE_H
