#ifndef READ_NET_H
#define READ_NET_H

#include "reader.h"

page_t* net_read(request_t *);
void    net_cache(request_t *, page_t *);

static reader_t read_net = {
  &net_read,
  &net_cache
};

#endif // READ_NET_H
