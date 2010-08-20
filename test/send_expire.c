#include <stdio.h>
#include <stdlib.h>

#include "read_file.h"
#include "zhongsou_net_udp.h"
#include "settings.h"

int
main(int argc, char **argv)
{
  page_t *page;
  request_t req;

  if (argc < 5) {
    printf("usage: %s host port url file\n", argv[0]);
    return(1);
  }

  cfg.udp_notify.idx=0;
  cfg.udp_notify.num=1;
  cfg.udp_notify.servers=calloc(1, sizeof(server_t));
  cfg.udp_notify.servers[0].host=argv[1];
  cfg.udp_notify.servers[0].port=atoi(argv[2]);

  req.url = argv[3];
  page = file_read_path(argv[4]);

  if (page != NULL) {
    printf("=======>expire: %s\n", argv[4]);
    udp_notify_expire(&req, page);
  }
  return 0;
}
