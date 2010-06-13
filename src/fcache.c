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

#include "Config.h"
#include "log.h"
#include "util.h"
#include "thread.h"

#include "fcache.h"
#include "settings.h"
#include "process.h"
#include "read_file.h"
#include "zhongsou_keyword.h"
#include "zhongsou_net_http.h"

static void
exit_on_sig(const int sig)
{
  tlog(FATAL, "signal: received=%d", sig);
  exit(EXIT_SUCCESS);
}

/* fastp: just handle, read mem/fs, write to client;
   slowp: possible auth, bypass, cache maitain
 */
static GThreadPool *fastp, *slowp;
typedef struct {
  struct evhttp_request *req;
  void *arg;

  request_t r;
  page_t *page;
  bool sent;
} req_ctx_t;

static void
send_page(req_ctx_t *ctx)
{
  struct evbuffer *buf;
  if ((buf = evbuffer_new()) == NULL) {
    tlog(ERROR, "failed to create response buffer");
  } else {
    tlog(DEBUG, "send page: %s", ctx->page->head.param);
    static char *content_type="text/html; charset=";
    const char *enc = cfg.page_encoding;
    if (enc==NULL||strlen(enc)<2) enc="UTF-8";
    char ct[strlen(content_type)+strlen(enc)+1];

    size_t len1=strlen(content_type), len2=strlen(enc);
    memcpy(ct, content_type, len1);
    memcpy(ct+len1, enc, len2);
    ct[len1+len2]='\0';

    evbuffer_add(buf, ctx->page->body, ctx->page->body_len);
    evhttp_add_header(ctx->req->output_headers, "Content-Type", ct);
    evhttp_add_header(ctx->req->output_headers, "Content-Encoding", "gzip");
    evhttp_send_reply(ctx->req, HTTP_OK, "OK", buf);
    ctx->sent = true;
    evbuffer_free(buf);
  }
}

static void
fast_process(gpointer data, gpointer user_data)
{
  req_ctx_t *ctx = data;
  struct evhttp_request *req = ctx->req;
  GError *error;
  char kw[strlen(req->uri)];

  ctx->r.domain = evhttp_find_header(ctx->req->input_headers, "Host");
  ctx->r.url = req->uri;
  ctx->r.keyword = find_keyword(req->remote_host, req->uri, kw);

  ctx->page=process_get(&ctx->r);

  if (ctx->page!=NULL && ctx->page->head.auth_type == AUTH_NO) {
    send_page(ctx);
  }

  tlog(DEBUG, "push %s to slow pool(page=%p)", ctx->req->uri, ctx->page);
  g_thread_pool_push(slowp, ctx, &error);
}

static void
slow_process(gpointer data, gpointer user_data)
{
  req_ctx_t *ctx = data;

  if (ctx->sent) {
    process_cache(&ctx->r, ctx->page);
  } else {
    if (ctx->page == NULL) {
      // bypass to upstream
      server_t *svr = next_server_in_group(&cfg.http);
      if (svr != NULL) {
        mmap_array_t data = {0, NULL};
        if (zs_http_pass_req(&data, ctx->req, svr->host, svr->port)) {
          if (data.data) tlog(DEBUG, "upstream:{\n%s\n}", data.data);
          struct evbuffer *buf;
          if ((buf = evbuffer_new()) == NULL) {
            tlog(ERROR, "failed to create response buffer");
          } else {
            evhttp_clear_headers(ctx->req->output_headers);
            //parse headers
            size_t header_len = 0, len;
            if (data.len>9 && strncmp(data.data, "HTTP/1.", 7)==0) {
              char *s = data.data, *line = NULL, *k, *v;
              do {
                line = strsep(&s, "\r\n");
                len  = strlen(line);
                header_len += len+1;
                if (*s == '\n') { s++; header_len++;}

                if (len>2 && strstr(line, ": ")!=NULL) {
                  k=strsep(&line, ": ");
                  v=(*line==' ')?line+1:line;
                  evhttp_add_header(ctx->req->output_headers, k, v);
                }
              } while(line!=NULL && len>0);
            }

            //send remaining
            evbuffer_add(buf, data.data+header_len, data.len-header_len);
            evhttp_send_reply(ctx->req, HTTP_OK, "OK", buf);
            ctx->sent = true;
            evbuffer_free(buf);
          }
          if (data.data!=NULL) free(data.data);
        }
      }
    } else {
      // auth
      if (ctx->page->head.auth_type != AUTH_NO) {
        char *igid;
        igid = zs_http_find_igid_by_cookie(ctx->req);
        if (igid == NULL) { //need auth, but no igid
          tlog(DEBUG, "igid not in cookie, but need auth");
          ctx->page = NULL;
        } else {
          ctx->page = process_auth(igid, ctx->page);
          free(igid);//igid come from strdup(), must free!
        }
      }

      if (ctx->page != NULL) {
        send_page(ctx);
        process_cache(&ctx->r, ctx->page);
      } else {
        // not authorized
        struct evbuffer *buf;
        if ((buf = evbuffer_new()) == NULL) {
          tlog(ERROR, "failed to create response buffer");
        } else {
          evbuffer_add_printf(buf, "reply");
          evhttp_send_reply(ctx->req, HTTP_MOVEPERM, "not permit", buf);
          evbuffer_free(buf);
        }
      }
    }
  }

  // finish
  free(ctx);
}

static void
init_fcache()
{
  GError *error;
  gint nth = cfg.num_threads/2;
  if (nth < 2) nth = 2;

  g_thread_init(NULL);
  fastp = g_thread_pool_new(fast_process, NULL, nth, false, &error);
  slowp = g_thread_pool_new(slow_process, NULL, nth, false, &error);
  if (fastp == NULL || slowp == NULL) {
    perror("can not initialize thread pool!");
    exit(1);
  }
}

void
page_handler(struct evhttp_request *req, void *arg)
{
  GError *error;
  req_ctx_t *ctx = calloc(1, sizeof(req_ctx_t));
  ctx->req  = req;
  ctx->arg  = arg;
  ctx->page = NULL;
  ctx->sent = false;
  tlog(DEBUG, "push %s to fast pool", req->uri);
  g_thread_pool_push(fastp, ctx, &error);
}

void
monitor_handler(struct evhttp_request *req, void *arg)
{
  struct evbuffer *buf;
  if ((buf = evbuffer_new()) == NULL) {
    printf("failed to create response buffer");
  } else {
    char *ht="ok";
    evbuffer_add(buf, ht, strlen(ht));
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
  }
}

int
main(int argc, char**argv)
{
  int c;
  struct rlimit rlim;

  signal(SIGINT, exit_on_sig);
  init_cfg();
  setbuf(stderr, NULL);

  /* process arguments */
  while (-1 != (c = getopt(argc, argv,
                           "C:"  /* config file */
                           "d"  /* goto daemon mode*/
                           "I:" /*interface to bind*/
                           "p:" /* TCP port number to listen on */
                           "s:" /* unix socket path to listen on */

                           "c:"  /* max simultaneous connections */
                           "P:"  /* file to hold pid */
                           "t:"  /* max thread connections */
                           ))) {
    switch (c) {
    case 'C': read_cfg(optarg); break;
    case 'd': cfg.daemon = 1;   break;
    case 'I': cfg.bind_addr = optarg; break;
    case 'p':
      cfg.port = (uint16_t)atoi(optarg);
      cfg.conn_type = tcp;
      break;
    case 's':
      cfg.socketpath = optarg;
      cfg.conn_type = domain_socket;
      break;
    case 'c': cfg.maxconns = atoi(optarg); break;
    case 'P': cfg.pid_file = optarg; break;
    case 't':
      cfg.num_threads = atoi(optarg);
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

  /*
   * If needed, increase rlimits to allow as many connections
   * as needed.
   */
  if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
    fprintf(stderr, "failed to getrlimit number of files\n");
    exit(EX_OSERR);
  } else {
    int maxfiles = cfg.maxconns;
    if (rlim.rlim_cur < maxfiles)
      rlim.rlim_cur = maxfiles;
    if (rlim.rlim_max < rlim.rlim_cur)
      rlim.rlim_max = rlim.rlim_cur;
    if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
      fprintf(stderr, "failed to set rlimit for open files. Try running as root or requesting smaller maxconns value.\n");
      exit(EX_OSERR);
    }
  }

  if (cfg.daemon) {
    if (sigignore(SIGHUP) == -1) {
      perror("Failed to ignore SIGHUP");
    }

    daemonize(1,1);
  }
  // files for log

  // Initialization
  if (sigignore(SIGPIPE) == -1) {
    perror("failed to ignore SIGPIPE; sigaction");
    exit(EXIT_FAILURE);
  }
  process_init();
  init_fcache();
  // keywords
  if (cfg.doamin_file != NULL)  read_domain(cfg.doamin_file);
  if (cfg.synonyms_file != NULL)read_synonyms(cfg.synonyms_file);

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
  evhttp_set_gencb(httpd, page_handler, NULL);

  tlog(DEBUG, "start done: %s, %d", cfg.bind_addr, cfg.port);
  event_dispatch();
  /* Not reached in this code as it is now. */
  evhttp_free(httpd);

  return EXIT_SUCCESS;
}
