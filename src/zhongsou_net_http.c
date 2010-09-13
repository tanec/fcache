#include <string.h>
#include <stdlib.h>
#include <sys/queue.h>

#include "zhongsou_net_http.h"
#include "log.h"

/**
  must free it
 */
char *
zs_http_find_keyword_by_uri(const char *orig_uri, char *out)
{
  char *s, c;
  int i;

  if (orig_uri == NULL || out == NULL || strcmp(orig_uri, "/fff")==0) {
    return NULL;
  }
  // try /keyword/xxx && /keyword?aaa
  s = (char *)orig_uri;
  if (*s=='/') s++;
  for(i=0;i<8192;i++) {
    c = *(s+i);
    if(c!='\0' && c!='/' && c!='?') {
      out[i] = c;
      out[i+1] = '\0';
    } else {
      break;
    }
  }
  return out;
}

static char *ks = "un_web=";
static size_t kslen = 7;//strlen(ks);

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
      ret  = igid==NULL?NULL:strdup(igid+kslen);
      if(ret!=NULL) {
        int p = strlen(ret) - 1;
        if (ret[p] == ';') ret[p] = '\0';
      }
    } else {
      strsep(&ck, " ");
    }
  } while(ck!=NULL && strlen(ck)>kslen && ret==NULL);
  free(ock);
  return ret;
}

bool
zs_http_pass_req(tbuf *resp, struct evhttp_request *c, const char *host, uint16_t port, const char *url, const char *path)
{
#define BUFLEN 16384
  char buf[BUFLEN+1];
  int len = 15, len1=0; //strlen("GET  HTTP/1.1\r\n")->15

  memset(buf, 0, BUFLEN+1);
  // copy params
  strcat(buf, "GET ");                       len1=strlen(c->uri);  if(len+len1>BUFLEN) return false;
  strcat(buf, c->uri);            len+=len1;
  strcat(buf, " HTTP/1.1\r\n");
  struct evkeyval *h;
  TAILQ_FOREACH(h, c->input_headers, next) { len1=4+strlen(h->key);if(len+len1>BUFLEN) return false;
    strcat(buf, h->key);          len+=len1;
    strcat(buf, ": ");                       len1=strlen(h->value);if(len+len1>BUFLEN) return false;
    strcat(buf, h->value);        len+=len1;
    strcat(buf, "\r\n");
  }                                          len1=17;              if(len+len1>BUFLEN) return false;
  strcat(buf, "Orignal-URL: ");   len+=len1;
  if(path) {                                 len1=1+strlen(path);  if(len+len1>BUFLEN) return false;
    strcat(buf, path);            len+=len1;
    strcat(buf, "$");
  }                                          len1=4+strlen(url);   if(len+len1>BUFLEN) return false;
  strcat(buf, url);//             len+=len1; // last one: not needed
  strcat(buf, "\r\n\r\n");
  tlog(DEBUG, "==>%s:%d request header:{\n%s\n}\n", host, port, buf);

  return tcp_read(resp, host, port, buf);
#undef BUFLEN
}
