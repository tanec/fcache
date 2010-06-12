#include <string.h>
#include <sys/queue.h>

#include "zhongsou_net_http.h"
#include "log.h"

char *
zs_http_find_keyword_by_uri(struct evhttp_request *req)
{
  static char *kw = "keyword=";
  char *uri, *s, *ret;
  uri = strdup(req->uri); // make a copy
  if (uri == NULL) {
    return NULL;
  } else { // try /path?keyword=xxx
    s = uri;
    if (strchr(s, '?') != NULL) strsep(&s, "?");
    s = strstr(s, kw);
    if (s != NULL && strchr(s, '&') != NULL) s=strsep(&s, "&");
    if (s != NULL) ret = strdup(s+strlen(kw));
  }
  if (ret == NULL) { // try /keyword/xxx
    s = uri;
    if (*s=='/') s++;
    if (*s!='\0' && strchr(s, '/')!=NULL) s=strsep(&s, "/");
    if (s!=NULL) ret=strdup(s);
  }
  free(uri);
  return ret;
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

bool
zs_http_pass_req(mmap_array_t *resp, struct evhttp_request *c, const char *host, uint16_t port)
{
#define BUFLEN 8192
  char buf[BUFLEN];

  memset(buf, 0, BUFLEN);
  // copy params
  strcat(buf, "GET ");
  strcat(buf, c->uri);
  strcat(buf, "\r\n");
  struct evkeyval *h;
  TAILQ_FOREACH(h, c->input_headers, next) {
    strcat(buf, h->key);
    strcat(buf, ": ");
    strcat(buf, h->value);
    strcat(buf, "\r\n");
  }
  strcat(buf, "Orignal-URL: ");
  strcat(buf, c->uri);
  strcat(buf, "\r\n\r\n");
  tlog(DEBUG, "==>request header:{\n%s\n}\n", buf);

  return tcp_read(resp, host, port, buf);
}
