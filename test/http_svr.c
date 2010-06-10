#include <event.h>
#include <evhttp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void
copycb(struct evhttp_request *req, void *arg)
{
  struct evbuffer *buf;
  if ((buf = evbuffer_new()) == NULL) {
    printf("failed to create response buffer");
  } else {
    char *ht="cpcpcpcpcpcp\n\n";
    evbuffer_add(buf, ht, strlen(ht));
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
  }
}

void
gencb(struct evhttp_request *req, void *arg)
{
  struct evbuffer *buf;
  if ((buf = evbuffer_new()) == NULL) {
    printf("failed to create response buffer");
  } else {
    char *ht="xxxxxxxxxxxx";
    evbuffer_add_printf(buf, "ht=%s\n", ht);
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
  }
}

int
main(int argc, char **argv)
{
  // initialize libraries
  struct evhttp *httpd;

  event_init();
  httpd = evhttp_start("0.0.0.0", atoi(argv[1]));
  if (httpd == NULL) {
    perror("evhttpd failed!");
    exit(1);
  }

  /* Set a callback for requests to "/specific". */
  evhttp_set_cb(httpd, "/c", copycb, NULL);

  /* Set a callback for all other requests. */
  evhttp_set_gencb(httpd, gencb, NULL);

  event_dispatch();
  /* Not reached in this code as it is now. */
  evhttp_free(httpd);

  return EXIT_SUCCESS;
}
