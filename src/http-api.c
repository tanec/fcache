#include "http-api.h"

void
http_init(void)
{
  curl_global_init(CURL_GLOBAL_ALL);
}

char *
http_get(const char *url)
{
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl != NULL) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }

  return NULL;
}

char *
http_post(const char *url, const char *data)
{

}
