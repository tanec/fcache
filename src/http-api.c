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

  struct curl_httppost *formpost = NULL;
  struct curl_httppost *lastptr  = NULL;

  va_list params;
  int i;
  va_start(params, pairs);
  for (i=0; i<pairs; i++) {
    char *k, *v;
    k = va_arg(params, char *);
    v = va_arg(params, char *);
    tlog(DEBUG, "http post: %s = %s", k, v);
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME,     k,
                 CURLFORM_COPYCONTENTS, v,
                 CURLFORM_END);
  }
  va_end(params);

  curl = curl_easy_init();
  if (curl != NULL) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    //curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, formpost);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)resp);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    curl_formfree(formpost);
    return res==CURLE_OK;
  }
  curl_formfree(formpost);
  return false;
}
