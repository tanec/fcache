#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include "http-api.h"
#include "log.h"

void
http_init(void)
{
  curl_global_init(CURL_GLOBAL_ALL);
}

static void *
myrealloc(void *ptr, size_t size)
{
  if(ptr)
    return realloc(ptr, size);
  else
    return malloc(size);
}

static size_t
write_memory_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  http_response_t *mem = (http_response_t *)data;

  mem->reply = myrealloc(mem->reply, mem->size + realsize + 1);
  if (mem->reply) {
    memcpy(&(mem->reply[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->reply[mem->size] = 0;
  }
  return realsize;
}

bool
http_get(const char *url, http_response_t *resp)
{
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl != NULL) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)resp);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res==CURLE_OK;
  }
  return false;
}

bool
http_post(const char *url, http_response_t *resp, int pairs, ...)
{
  CURL *curl;
  CURLcode res;

  int i, len=0;
  char *k[pairs], *v[pairs], *vv[pairs];
  va_list params;
  va_start(params, pairs);
  for (i=0; i<pairs; i++) {
    k[i] = va_arg(params, char *);
    v[i] = va_arg(params, char *);
    vv[i]= curl_easy_escape(curl, v[i], 0);
    tlog(DEBUG, "http post: %s = %s(%s)", k[i], v[i], vv[i]);
    len += 4+strlen(k[i])+strlen(vv[i]);
  }
  va_end(params);

  char data[len];
  memset(data, 0, len);
  for (i=0; i<pairs; i++) {
    strcat(data, "&");
    strcat(data, k[i]);
    strcat(data, "=");
    strcat(data, vv[i]);
    curl_free(vv[i]);
  }
  tlog(DEBUG, "http post: data = %s", data);

  curl = curl_easy_init();
  if (curl != NULL) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)resp);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res==CURLE_OK;
  }
  return false;
}

// out should be enough to hold result
bool
http_unescape(const char *in, char *out)
{
  int   len = -1;
  char *res = NULL;
  CURL *curl;
  bool ret = false;

  curl = curl_easy_init();
  if (curl != NULL) {
    res = curl_easy_unescape(curl, in, 0, &len);
    if (res != NULL && len > 0) {
      memcpy(out, res, len+1);
      ret = true;
    }
    curl_easy_cleanup(curl);
    return ret;
  }
  return false;
}
