#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iconv.h>

#include <event.h>
#include <evhttp.h>

#include "log.h"

static void
exit_on_sig(const int sig)
{
  exit(EXIT_SUCCESS);
}

static struct cfg_s {
  const char *bind_addr;
  uint16_t port;

  const char *monitor_path;
  const char *status_path;
}
cfg = {
  "0.0.0.0", 8012,
  "/ishc/mon", "/ishc/status"
};

int i_shc_handler(evbuffer *buf, char *fullurl);

void
request_handler(struct evhttp_request *req, void *arg)
{
  struct evbuffer *buf;
  if ((buf = evbuffer_new()) == NULL) {
    tlog(ERROR, "failed to create response buffer");
  } else {
    i_shc_handler(buf, req->uri);
    evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=gbk");
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
  }
}

void
monitor_handler(struct evhttp_request *req, void *arg)
{
  struct evbuffer *buf;
  if ((buf = evbuffer_new()) == NULL) {
    printf("failed to create response buffer");
  } else {
    const char *ht="ok";
    evbuffer_add(buf, ht, strlen(ht));
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
  }
}

int
main(int argc, char**argv)
{
  int c;

  signal(SIGINT, exit_on_sig);
  if (sigignore(SIGPIPE) == -1) {
    perror("failed to ignore SIGPIPE; sigaction");
    exit(EXIT_FAILURE);
  }

  /* process arguments */
  while (-1 != (c = getopt(argc, argv,
                           "i:" /* interface to bind */
                           "p:" /* TCP port number to listen on */
                           "M:" /* uri of monitor */
                           "S:" /* uri of status  */
                           ))) {
    switch (c) {
    case 'i': cfg.bind_addr    = optarg; break;
    case 'p': cfg.port         = (uint16_t)atoi(optarg); break;
    case 'M': cfg.monitor_path = optarg; break;
    case 'S': cfg.status_path  = optarg; break;
    default: return 1;
    }
  }

  // initialize libraries
  struct evhttp *httpd;

  event_init();
  httpd = evhttp_start(cfg.bind_addr, cfg.port);
  if (httpd == NULL) {
    perror("evhttpd failed!");
    exit(1);
  }

  /* Set a callback for requests to "/specific". */
  if (cfg.monitor_path != NULL)
    evhttp_set_cb(httpd, cfg.monitor_path, monitor_handler, NULL);

  /* Set a callback for all other requests. */
  evhttp_set_gencb(httpd, request_handler, NULL);

  tlog(DEBUG, "start done: %s, %d", cfg.bind_addr, cfg.port);
  event_dispatch();
  /* Not reached in this code as it is now. */
  evhttp_free(httpd);

  return EXIT_SUCCESS;
}
