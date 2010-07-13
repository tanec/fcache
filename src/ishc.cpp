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

#include <event.h>
#include <evhttp.h>

#include <glib.h>

#include "util.h"
#include "log.h"


static void
exit_on_sig(const int sig)
{
  exit(EXIT_SUCCESS);
}

static GThreadPool *tpool;
static struct cfg_s {
  int num_threads;
  bool daemon;

  const char *bind_addr;
  uint16_t port;

  const char *monitor_path;
  const char *status_path;
}
cfg = {
  4, false,
  "127.0.0.1", 8012,
  "/ishc/mon", "/ishc/status"
};

typedef struct {
  struct evhttp_request *req;
  void *arg;
} req_ctx_t;

static inline bool
is_str_empty(const char *s)
{
  return s==NULL || strlen(s)<1;
}

int i_shc_handler(evbuffer *buf, char *fullurl);

static void
process(gpointer data, gpointer user_data)
{
  req_ctx_t *ctx = (req_ctx_t *)data;
  GError *error;

  struct evbuffer *buf;
  if ((buf = evbuffer_new()) == NULL) {
    tlog(ERROR, "failed to create response buffer");
  } else {
    i_shc_handler(buf, ctx->req->uri);
    evhttp_add_header(ctx->req->output_headers, "Content-Type", "text/html; charset=gbk");
    evhttp_send_reply(ctx->req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
  }

  free(ctx);
}

void
request_handler(struct evhttp_request *req, void *arg)
{
  GError *error;
  req_ctx_t *ctx = (req_ctx_t *)calloc(1, sizeof(req_ctx_t));
  ctx->req  = req;
  ctx->arg  = arg;

  tlog(DEBUG, "push %s to thread pool", req->uri);
  g_thread_pool_push(tpool, ctx, &error);
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
                           "d"  /* goto daemon mode  */
                           "t:" /* max thread connections */
                           "i:" /* interface to bind */
                           "p:" /* TCP port number to listen on */
                           "M:" /* uri of monitor */
                           "S:" /* uri of status  */
                           ))) {
    switch (c) {
    case 'd': cfg.daemon       = true;   break;
    case 'i': cfg.bind_addr    = optarg; break;
    case 'p': cfg.port         = (uint16_t)atoi(optarg); break;
    case 'M': cfg.monitor_path = optarg; break;
    case 'S': cfg.status_path  = optarg; break;
    case 't': cfg.num_threads  = atoi(optarg);
      if (cfg.num_threads <= 0) {
        fprintf(stderr, "Number of threads must be greater than 0\n");
        return 1;
      }
      if (cfg.num_threads > 64) {
        fprintf(stderr, "WARNING: Setting a high number of worker"
                "threads is not recommended.\n"
                " Set this value to the number of cores in"
                " your machine or less.\n");
      }
      break;
    default: return 1;
    }
  }

  if (cfg.daemon) {
    if (sigignore(SIGHUP) == -1) {
      perror("Failed to ignore SIGHUP");
    }

    daemonize(1,1);
  }

  // Initialization
  {
    GError *error;
    if (cfg.num_threads < 2) cfg.num_threads = 2;
    if (cfg.num_threads >64) cfg.num_threads = 64;

    g_thread_init(NULL);
    tpool = g_thread_pool_new(process, NULL, cfg.num_threads, false, &error);
    if (tpool == NULL) {
      perror("can not initialize thread pool!");
      exit(EXIT_FAILURE);
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
