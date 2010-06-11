#include <string.h>

#include "zhongsou_net_http.h"

char *
zs_http_find_keyword_by_uri(struct evhttp_request *req)
{

}

static char *ks = "igid=";
static size_t kslen = 5;//strlen(ks);

char *
zs_http_find_igid_by_cookie(struct evhttp_request *req)
{
  const char *cookie;
  char *ck, *ock, *ret=NULL;
  cookie = evhttp_find_header(req->input_headers, "Cookie");
  if (cookie == NULL || strlen(cookie)<kslen) return NULL;

  ck = strdup(cookie);
  ock=ck;
  do {
    if (strncmp(ck, ks, kslen) == 0) {
      char *igid = strsep(&ck, " ");
      ret  = igid==NULL?NULL:strdup(igid);
      *(ret+strlen(ret)-1)='\0';
    } else {
      strsep(&ck, " ");
    }
  } while(ck!=NULL && strlen(ck)>kslen && ret==NULL);
  free(ock);
  return ret;
}
