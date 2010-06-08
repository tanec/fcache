#ifndef HTTPAPI_H
#define HTTPAPI_H

#include <curl/curl.h>

typedef struct {
  char *reply;
  size_t size;
} http_response_t;

void http_init(void);
bool http_get(const char *, http_response_t *);
bool http_post(const char *, const char *, http_response_t *);

#endif // HTTPAPI_H
