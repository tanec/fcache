#include <stdio.h>
#include <stdlib.h>

#include "zhongsou_net_auth.h"
#include "read_file.h"
#include "settings.h"

int main(int argc, char **argv)
{
  char *igid;
  page_t *page;

  if(argc < 4) {
    printf("usage: %s url igid page\n", argv[0]);
    exit(1);
  }

  cfg.auth.idx=0;
  cfg.auth.num=1;
  cfg.auth.servers=calloc(1, sizeof(server_t));
  cfg.auth.servers[0].url=argv[1];

  igid = argv[2];
  page = file_read_path(argv[3]);
  if (page!=NULL) {
    bool res = auth_http(igid, page->head.keyword, 1, page->head.param);
    printf("%s on %s -> %d\n", igid, argv[2], res);
  }

  return 0;
}
