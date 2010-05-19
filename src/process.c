#include <stdbool.h>
#include <stdio.h>

#include "process.h"
#include "reader.h"
#include "read_mem.h"
#include "read_file.h"
#include "read_net.h"
#include "md5.h"

void
md5_dir(zhongsou_t *zt)
{
  if ((zt->set_mask&0x1) == 0) {
    md5_digest(NULL, 0, zt->dig_dir);
    zt->set_mask &= 0x1;
  }
}

void
md5_file(zhongsou_t *zt)
{
  if ((zt->set_mask&0x2) == 0) {
    md5_digest(NULL, 0, zt->dig_file);
    zt->set_mask &= 0x2;
  }
}

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
