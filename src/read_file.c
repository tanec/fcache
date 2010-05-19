#include <stdio.h>
#include <string.h>

#include "read_file.h"
#include "be_read.h"
#include "config.h"

page_t *
file_read(request_t *req)
{
  zhongsou_t zt = req->inner;
  char path[39 + strlen(cfg.base_dir)]; //len(md5)+len('/'s)+'\0' = 32+6+1

  md5_dir(&zt);
  sprintf(path,
	  "%s/%02x%02x/%02x%02x/%02x%02x/%02x%02x/%02x%02x%02x%02x%02x%02x%02x%02x/",
	  cfg.base_dir,
	  zt.dig_dir[0], zt.dig_dir[1], zt.dig_dir[2], zt.dig_dir[3],
	  zt.dig_dir[4], zt.dig_dir[5], zt.dig_dir[6], zt.dig_dir[7],
	  zt.dig_dir[8], zt.dig_dir[9], zt.dig_dir[10], zt.dig_dir[11],
	  zt.dig_dir[12], zt.dig_dir[13], zt.dig_dir[14], zt.dig_dir[15]);

    return NULL;
}

void
file_cache(request_t *req, page_t *page){} // do nothing
