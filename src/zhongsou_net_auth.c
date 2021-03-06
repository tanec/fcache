#include <string.h>
#include <stdlib.h>

#include "zhongsou_net_auth.h"
#include "http-api.h"
#include "settings.h"
#include "log.h"

bool
auth_http(const char *igid, const char *keyword, uint32_t auth_type, const char *json)
{
  bool ret = false;
  http_response_t resp = {NULL, 0};
  server_t *svr = NULL;

  svr = next_server_in_group(&cfg.auth);
  if (svr!=NULL && http_post(svr->url, &resp, 3, "igid", igid, "keyword", keyword, "auth", json)) {
    // strlen("{"PageAuth":0,"GroupAuth":0}")=28
    if (resp.reply!=NULL && resp.size>15)
      ret = (*(resp.reply+12) != '0') && (*(resp.reply+26) != '0');
    tlog(DEBUG, "result=%s %d", resp.reply, ret);
  }
  if (resp.reply != NULL) free(resp.reply);
  return ret;
}
