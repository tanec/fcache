#include <string.h>
#include <stdlib.h>

#include "zhongsou_net_auth.h"
#include "http-api.h"
#include "settings.h"
#include "log.h"

bool
auth_http(char *igid, char *keyword, uint32_t auth_type, char *json)
{
  bool ret = false;
  http_response_t resp = {NULL, 0};
  int sz = 32+strlen(json)+strlen(keyword)+strlen(igid);
  char data[sz];
  server_t *svr = NULL;

  memset(data, 0, sz);
  snprintf(data, sz,
           "igid=%s&keyword=%s&auth=%s",
           igid, keyword, json);
  svr = next_server_in_group(&cfg.auth);
  if (svr!=NULL && http_post(svr->url, &resp, 3, "igid", igid, "keyword", keyword, "auth", json)) {
    tlog(DEBUG, "send %s to %s", data, svr->url);
    tlog(DEBUG, "result=%s", resp.reply);
    // strlen("{"PageAuth":0,"GroupAuth":0}")=28
    if (resp.reply!=NULL && resp.size>15)
      ret = (*(resp.reply+12) != '0');
  }
  if (resp.reply != NULL) free(resp.reply);
  return ret;
}
