#include <curl/curl.h>
#include "curl-api.h"

void start() {
    curl_easy_init();
}

void stop() {
    curl_easy_cleanup(0);
}

char* getStr(char* url) {

}

char * getBin(char* url) {

}

char* post(char* url) {

}
