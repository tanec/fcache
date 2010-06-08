#include <string.h>
#include <stdlib.h>

#include "zhongsou_net_auth.h"
#include "http-api.h"
#include "settings.h"

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
  if (svr!=NULL && http_post(svr->url, data, &resp)) {
    // strlen("{}")=//TODO
    if (resp.reply!=NULL && resp.size>15)
      ret = (*(resp.reply+8) != '0');
  }
  if (resp.reply != NULL) free(resp.reply);
  return ret;
}
