#include <sys/queue.h>
#include <event.h>
#include <evhttp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <string.h>

#include "zhongsou_net_http.h"

static char *host= NULL;
uint16_t port=-1;

void
proxycb(struct evhttp_request *req, void *arg)
{
  struct evbuffer *buf;

  if ((buf = evbuffer_new()) == NULL) {
    printf("failed to create response buffer");
  } else {
    tbuf resp = {0, NULL};
    zs_http_pass_req(&resp, req, host, port, req->uri);
    printf("resp: {%d, %p}\n", resp.len, resp.data);
    if (resp.data!=NULL) printf("%s\n", resp.data);
    evbuffer_add(buf, resp.data, resp.len);
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
    if (resp.data!=NULL) free(resp.data);
  }
}

int
main(int argc, char **argv)
{
  // initialize libraries
  struct evhttp *httpd;

  if (sigignore(SIGPIPE) == -1) {
    perror("failed to ignore SIGPIPE; sigaction");
    exit(EXIT_FAILURE);
  }

  host = argv[2];
  port = atoi(argv[3]);
  event_init();
  httpd = evhttp_start("0.0.0.0", atoi(argv[1]));
  if (httpd == NULL) {
    perror("evhttpd failed!");
    exit(1);
  }

  /* Set a callback for all other requests. */
  evhttp_set_gencb(httpd, proxycb, NULL);

  event_dispatch();
  /* Not reached in this code as it is now. */
  evhttp_free(httpd);

  return EXIT_SUCCESS;
}
