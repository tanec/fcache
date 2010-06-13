#ifndef READ_FILE_H
#define READ_FILE_H

#include "reader.h"

page_t* file_get(md5_digest_t *, md5_digest_t *);
page_t* file_read_path(char *path);

#endif // READ_FILE_H
