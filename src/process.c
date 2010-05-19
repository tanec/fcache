#include <stdbool.h>
#include <stdio.h>

#include "process.h"
#include "reader.h"
#include "read_mem.h"
#include "read_file.h"
#include "read_net.h"

extern reader_t read_mem, read_file, read_net;

void
process(request_t *req, response_t *resp)
{
  bool cache_after_response = false;
  page_t *data = NULL;

  data = (*read_mem.read)(req);
  if (data == NULL) {
    data = (*read_file.read)(req);
    cache_after_response = true;
  }
  if (data == NULL) {
    data = (*read_net.read)(req);
    cache_after_response = true;
  }
  if (data != NULL) {
    //TODO
    if (cache_after_response) {
      (*read_mem.cache)(req, data);
    }
  }
}
