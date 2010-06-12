#ifndef ZHONGSOU_NET_HTTP_H
#define ZHONGSOU_NET_HTTP_H

#include <stdbool.h>
#include <event.h>
#include <evhttp.h>

#include "util.h"

char * zs_http_find_keyword_by_uri(const char *);
char * zs_http_find_igid_by_cookie(struct evhttp_request *);
bool zs_http_pass_req(mmap_array_t *, struct evhttp_request *, const char *, uint16_t);

#endif // ZHONGSOU_NET_HTTP_H
