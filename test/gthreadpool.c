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
#include <ctype.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <event.h>
#include <evhttp.h>

#include <glib.h>

#include "Config.h"
#include "log.h"
#include "util.h"
#include "thread.h"
#include "http-api.h"

#include "fcache.h"
#include "settings.h"
#include "process.h"
#include "smalloc.h"

static void
exit_on_sig(const int sig)
{
  exit(EXIT_SUCCESS);
}

/* fastp: just handle, read mem/fs
   slowp: possible auth, bypass
   cache: cache maitain
 */
static GThreadPool *fastp, *slowp, *cache;
typedef struct {
  // from client
  struct evhttp_request *client_req;
  void *arg;

  char data[163840];

} req_ctx_t;


static void
fast_process(gpointer data, gpointer user_data)
{
  char xx[8192];
  GError *error;
  memset(xx, 'x', 8192);
  if (current_time_millis() % 2) {
    usleep(88);
    g_thread_pool_push(cache, data, &error);
  } else {
    usleep(125);
    g_thread_pool_push(slowp, data, &error);
  }
}

static void
slow_process(gpointer data, gpointer user_data)
{
  GError *error;
  req_ctx_t *ctx = data;

  if (current_time_millis()%2) {
    sleep(1);
  }

  g_thread_pool_push(cache, ctx, &error);
}

static void
cache_process(gpointer data, gpointer user_data)
{
  if(data) sfree(data);
}

static void
init_fcache()
{
  GError *error;
  gint nth = 200;
  if (nth < 2) nth = 2;

  g_thread_init(NULL);
  fastp = g_thread_pool_new(fast_process, NULL, nth, false, &error);
  slowp = g_thread_pool_new(slow_process, NULL, nth, false, &error);
  cache = g_thread_pool_new(cache_process,NULL, 4, false, &error);
  if (fastp == NULL || slowp == NULL || cache == NULL) {
    perror("can not initialize thread pool!");
    exit(1);
  }
}

#define CN 1024
//req_ctx_t *ctxs[CN];
uint64_t n=0;

void
page_handler(struct evhttp_request *req, void *arg)
{
  GError *error;
  req_ctx_t *ctx =NULL;
  struct evbuffer *buf;

  // enter from evhttpd
  if ((ctx = smalloc(sizeof(req_ctx_t))) == NULL) { return; } else {memset(ctx, 4, sizeof(req_ctx_t));}
  //ctx = ctxs[(n++)%CN];
  g_thread_pool_push(fastp, ctx, &error);

  
  if ((buf = evbuffer_new()) == NULL) {
    printf("failed to create response buffer");
  } else {
    evbuffer_add_printf(buf, "alloced: %lu bytes\n", smalloc_used_memory());
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
  }
}


int
main(int argc, char**argv)
{
  signal(SIGINT, exit_on_sig);

  int i;
  init_fcache();
  //for (i=0; i<CN; i++) if((ctxs[i]=smalloc(sizeof(req_ctx_t)))==NULL) exit(3); else memset(ctxs[i], 3, sizeof(req_ctx_t));
  // initialize libraries
  struct evhttp *httpd;
  event_init();
  httpd = evhttp_start("0.0.0.0", atoi(argv[1]));
  if (httpd == NULL) {
    perror("evhttpd failed!");
    exit(1);
  }

  /* Set a callback for all other requests. */
  evhttp_set_gencb(httpd, page_handler, NULL);
  event_dispatch();
  /* Not reached in this code as it is now. */
  evhttp_free(httpd);

  return EXIT_SUCCESS;
}
