#ifndef HTTPAPI_H
#define HTTPAPI_H

#include <curl/curl.h>

typedef struct {
  char *reply;
  size_t size;
} http_response_t;

void http_init(void);
bool http_get(const char *, http_response_t *);
bool http_post(const char *, http_response_t *, int, ...);
bool http_unescape(const char *, char *);
bool http_escpe(const char *, char *);

#endif // HTTPAPI_H
