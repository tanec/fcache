#ifndef ZHONGSOU_NET_HTTP_H
#define ZHONGSOU_NET_HTTP_H

#include <event.h>
#include <evhttp.h>

char * zs_http_find_keyword_by_uri(struct evhttp_request *);
char * zs_http_find_igid_by_cookie(struct evhttp_request *);

#endif // ZHONGSOU_NET_HTTP_H
