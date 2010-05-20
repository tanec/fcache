#include <stdbool.h>
#include <string.h>

#include "process.h"
#include "reader.h"
#include "read_mem.h"
#include "read_file.h"
#include "md5.h"

typedef enum {
  mem,
  fs
} ds_t;

void
md5_dir(zhongsou_t *zt)
{
  if ((zt->set_mask&0x1) == 0) {
    size_t len1 = strlen(zt->domain);
    size_t len2 = strlen(zt->keyword);
    char d[len1+len2+1];
    memcpy(d, zt->domain, len1);
    memcpy(d+len1, zt->keyword, len2);
    d[len1+len2] = 0;
    md5_digest(d, len1+len2, zt->dig_dir);
    zt->set_mask |= 0x1;
  }
}

void
md5_file(zhongsou_t *zt)
{
  if ((zt->set_mask&0x2) == 0) {
    md5_digest(zt->url, strlen(zt->url), zt->dig_file);
    zt->set_mask |= 0x2;
  }
}

bool
is_expire(page_t *page)
{

}

void
process_init()
{
  mem_init();
}

void
process(request_t *req, response_t *resp)
{
  page_t *page = mem_get(req);
  ds_t from = mem;

  if (page == NULL) {
    page = file_get(req);
    from = fs;
  }

  if (page == NULL) {
    // pass to upstream servers
  } else {
    bool expire;
    //TODO: send to client

    // send done
    expire = is_expire(page);
    if (from == fs) {
        page_free(mem_set(req, page));
      if (expire) {
	//udp: notify
      }
  } else if (expire && from == mem) {
      page_t *p1 = file_get(req);
      if (p1 == NULL) { // not in fs: delete
          page_free(mem_del(req));
      } else if (is_expire(p1)) { // expire in fs
          page_free(p1);
          // udp: notify
      } else { // valid on fs
          page_free(mem_set(req, p1));
      }
    }
  }
}
