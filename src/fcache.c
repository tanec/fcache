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
#include "read_file.h"
#include "zhongsou_keyword.h"
#include "zhongsou_net_http.h"
#include "zhongsou_monitor.h"
#include "zhongsou_page_save.h"

static void
exit_on_sig(const int sig)
{
  tlog(FATAL, "signal: received=%d", sig);
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

  // pass to other function
  request_t req;

  page_t *page;
  // response
  int              resp_code;
  const char      *resp_reason;
  struct evbuffer *resp_buf;
} req_ctx_t;

typedef struct ctx_node_s ctx_node_t;
struct ctx_node_s {
  req_ctx_t  *ctx;
  ctx_node_t *next;
};

static const char *send_start_path = "/fcache/send/start";
static char send_start_request[512] = {0};
static ctx_node_t ctx_lst = {NULL, NULL};

static void
send_list_push(req_ctx_t *ctx)
{
  ctx_node_t *node, *next;

  node = calloc(1, sizeof(ctx_node_t));
  if (node == NULL) return;

  node->ctx  = ctx;
  while(1) {
    next = ctx_lst.next;
    node->next = next;
    if(next == SYNC_CAS(&ctx_lst.next, next, node)) break;
  }

  // notify
  tbuf resp = {0, NULL};
  const char *host = cfg.bind_addr;

  if (strcmp(cfg.bind_addr, "0.0.0.0") == 0)
    host= "127.0.0.1";
  if (send_start_request[0] == 0)
    sprintf(send_start_request, "GET %s HTTP/1.0\r\n\r\n", send_start_path);
  tcp_read(&resp, host, cfg.port, send_start_request);
  if (resp.data != NULL) free(resp.data);
}

static req_ctx_t *
send_list_pop(void)
{
  ctx_node_t *node = NULL;
  while(1) {
    node = ctx_lst.next;
    if (node == NULL) return NULL;
    if (node == SYNC_CAS(&ctx_lst.next, node, node->next)) break;
  }
  req_ctx_t *ctx = (node==NULL)?NULL:node->ctx;
  free(node);
  return ctx;
}

static void
send_page(req_ctx_t *ctx)
{
  tlog(DEBUG, "send page: %s", ctx->page->head.param);
  static char *content_type="text/html; charset=";
  const char *enc = cfg.page_encoding;
  if (enc==NULL||strlen(enc)<2) enc="UTF-8";
  char ct[strlen(content_type)+strlen(enc)+1];

  size_t len1=strlen(content_type), len2=strlen(enc);
  memcpy(ct, content_type, len1);
  memcpy(ct+len1, enc, len2);
  ct[len1+len2]='\0';

  bool acceptgzip = true;
  {
    const char *ae = evhttp_find_header(ctx->client_req->input_headers, "Accept-Encoding");
    if (ae==NULL || strstr(ae, "gzip")==NULL) {
      acceptgzip = false;
    }
  }

  evhttp_add_header(ctx->client_req->output_headers, "Content-Type", ct);
  if (acceptgzip)
    evhttp_add_header(ctx->client_req->output_headers, "Content-Encoding", "gzip");

  ctx->resp_code   = HTTP_OK;
  ctx->resp_reason = "OK";
  if (acceptgzip) {
    evbuffer_add(ctx->resp_buf, ctx->page->body, ctx->page->body_len);
  } else {
    // uncompress
    tlog(INFO, "gunzip for %s, URL: %s%s, UA: %s",
	 evhttp_find_header(ctx->client_req->input_headers, "X-Forwarded-For"),
	 evhttp_find_header(ctx->client_req->input_headers, "Host"),
	 ctx->client_req->uri,
	 evhttp_find_header(ctx->client_req->input_headers, "User-Agent"));
    tbuf out = {0, NULL};
    zlib_gunzip(&out, ctx->page->body, ctx->page->body_len);
    evbuffer_add(ctx->resp_buf, out.data, out.len);
    tbuf_close(&out);
  }
  send_list_push(ctx);
}

static void
send_redirect(req_ctx_t *ctx, const char *url)
{
  tlog(DEBUG, "send redirect: %s", url);
  evhttp_add_header(ctx->client_req->output_headers, "Connection", "close");
  evhttp_add_header(ctx->client_req->output_headers, "Content-Length", "0");
  evhttp_add_header(ctx->client_req->output_headers, "Location", url);

  ctx->resp_code   = HTTP_MOVETEMP;
  ctx->resp_reason = "Moved Temporarily";
  send_list_push(ctx);
}

static void
pass_to_upstream(req_ctx_t *ctx)
{
  server_t *svr = next_server_in_group(&cfg.http);
  if (svr != NULL) {
    uint64_t s;
    int      slot;
    bool     result;
    tbuf data = {0, NULL};
    char path[128];

    s      = current_time_millis();
    slot   = process_upstream_start();
    file_path(path, md5_dir(&ctx->req), md5_file(&ctx->req));
    result = zs_http_pass_req(&data, ctx->client_req, svr->host, svr->port, ctx->req.url_orig, path);
    process_upstream_end(slot, s, result);

    if (result) {
      tlog(DEBUG, "upstream response length: %d", data.len);

      evhttp_clear_headers(ctx->client_req->output_headers);
      //parse headers
      ctx->resp_code   = HTTP_OK;
      ctx->resp_reason = "OK";
      size_t header_len = 0, len;
      bool chunked = false;
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
            if (strcmp(k, "Transfer-Encoding")==0 && strcmp(v, "chunked")==0)
              chunked = true;
            else
              evhttp_add_header(ctx->client_req->output_headers, k, v);
          } else if (strncmp(line, "HTTP/1.", 7)==0) {
            strsep(&line, " "); // discard "HTTP/1.X"
            ctx->resp_code   = atoi(strsep(&line, " "));
            ctx->resp_reason = request_store(&ctx->req, 1, line);
          }
        } while(line!=NULL && len>0);
      }

      if (chunked) {
	char *s = data.data+header_len, *line = NULL;
	for(;;) {
	  line = strsep(&s, "\r\n");
	  if (line == NULL) break;
	  if (*s == '\n') { s++; }
	  len  = strlen(line);
	  if (len==1 && line[0]=='0') break;
	  len = strtol(line, NULL, 16);
	  if (len > 0) {
	    evbuffer_add(ctx->resp_buf, s, len);
	    s += len + 2;
	  }
	}
      } else {
	//send remaining
	evbuffer_add(ctx->resp_buf, data.data+header_len, data.len-header_len);
      }

      if (data.data!=NULL) free(data.data);
    }
  } else {
    // no upstream server
    ctx->resp_code   = HTTP_SERVUNAVAIL;
    ctx->resp_reason = "serv unavail";
    evbuffer_add_printf(ctx->resp_buf, "fcache cannot find a usable np server!");
  }
  send_list_push(ctx);
}

static inline bool
is_str_empty(const char *s)
{
  return s==NULL || strlen(s)<1;
}

static inline const char *
find_igid(req_ctx_t *ctx)
{
  if (ctx->req.igid == NULL) {
    char *igid = NULL;
    igid = zs_http_find_igid_by_cookie(ctx->client_req);
    tlog(DEBUG, "igid from cookie: %s", igid);
    if (igid != NULL) {
      ctx->req.igid = request_store(&ctx->req, 1, igid);
      free(igid);//igid come from strdup(), must free!
    }
  }
  return ctx->req.igid;
}

static void
fast_process(gpointer data, gpointer user_data)
{
  req_ctx_t *ctx = data;
  struct evhttp_request *c = ctx->client_req;
  GError *error;
  char *s;
  const char *igid;
  bool kw_from_uri = true;

  { // host
    s = (char *)evhttp_find_header(c->input_headers, "Host");
    if (is_str_empty(s)) s = c->remote_host;
    ctx->req.host = request_store(&ctx->req, 1, s);
    strtolower((char *)ctx->req.host);
  }
  { // keyword: extract from uri, then transform
    char buf[8192]={0};
    s = zs_http_find_keyword_by_uri(c->uri, buf);
    tlog(DEBUG, "keyword from uri: %s -> %s", c->uri, s);

    if (s != NULL) {
      char newS[strlen(s)+1];
      if (http_unescape(s, newS)) {
        ctx->req.keyword = request_store(&ctx->req, 1, newS);
      } else {
        ctx->req.keyword = request_store(&ctx->req, 1, s);
      }
    }

    if (is_str_empty(ctx->req.keyword)) ctx->req.keyword = "/";
    strtolower((char *)ctx->req.keyword);
    const char *oldkw = ctx->req.keyword;
    ctx->req.keyword = find_keyword(ctx->req.host, ctx->req.keyword);
    kw_from_uri = strcmp(oldkw, ctx->req.keyword)==0;
  }
  {//url: host+uri, discard "http://"; parse "/fff", "&fff=1"
    if (kw_from_uri) {
      char *s = c->uri;
      if (*s == '/') s++;
      while(*s!='/' && *s!='\0' && *s!='?') {
	if (isupper(*s)) *s = tolower(*s);
	s++;
      }
    }
    size_t len = strlen(c->uri);
    char uri[len+1];
    memcpy(uri, c->uri, len+1);
    if (len>=4 && strcmp(c->uri+len-4, "/fff")==0) {
      uri[len-4] = '\0';
      ctx->req.force_refresh = true;
    } else {
      if (len>=6 && strcmp(c->uri+len-6, "&fff=1")==0) {
        uri[len-6] = '\0';
        ctx->req.force_refresh = true;
      }
    }
    ctx->req.url      = request_store(&ctx->req, 2, ctx->req.host, uri);
    ctx->req.url_orig = request_store(&ctx->req, 2, ctx->req.host, c->uri);
    char *s = (char *)ctx->req.url;
    len = strlen(s);    if (s[len-1]=='/') s[len-1] ='\0';
    s = (char *)ctx->req.url_orig;
    len = strlen(s);    if (s[len-1]=='/') s[len-1] ='\0';
  }

  tlog(DEBUG, "host=%s, keyword=%s, url=%s", ctx->req.host, ctx->req.keyword, ctx->req.url);

  ctx->page=process_get(&ctx->req);
  if (ctx->req.force_refresh) {
    if (ctx->page != NULL) ctx->page->head.valid = 0;
    mem_release(ctx->page);
    ctx->page = NULL;
  } else if(ctx->page!=NULL) {
    uint64_t time_save;
    bool mfs_np_ok = cfg.base_dir_ok;
    if (mfs_np_ok) mfs_np_ok = is_server_available(&cfg.http);
    if(mfs_np_ok && current_time_millis() > ctx->page->head.time_dead) {
      tlog(ERROR, "static page dead: %s", ctx->page->head.param);
      mem_release(ctx->page);
      ctx->page = NULL;
    } else if((igid=find_igid(ctx))!=NULL &&
	      ctx->page->head.ig!=NULL &&
	      strcmp(igid, ctx->page->head.ig)==0) {
      //owner: pass to upstream
      mem_release(ctx->page);
      ctx->page = NULL;
    } else if (mfs_np_ok
	       &&(time_save=page_save_time(ctx->page->head.page_no))!=UNKNOWN_SAVE_TIME
	       && time_save>ctx->page->head.time_create) {
      tlog(DEBUG, "static page gen(%llu) before save(%llu): %s", ctx->page->head.time_create, time_save, ctx->page->head.param);
      mem_release(ctx->page);
      ctx->page = NULL;
    }
  }

  if (ctx->page!=NULL && ctx->page->head.auth_type == AUTH_NO) {
    send_page(ctx);
  } else {
    tlog(DEBUG, "push %s to slow pool(page=%p)", ctx->client_req->uri, ctx->page);
    g_thread_pool_push(slowp, ctx, &error);
  }
}

static void
slow_process(gpointer data, gpointer user_data)
{
  req_ctx_t *ctx = data;

  if (ctx->page == NULL) {
    // bypass to upstream
    tlog(DEBUG, "pass2upstream: %s", ctx->req.url);
    pass_to_upstream(ctx);
  } else {
    // auth
    if (ctx->page->head.auth_type != AUTH_NO) {
      const char *igid;
      igid = find_igid(ctx);
      if (igid == NULL) { //need auth, but no igid
        tlog(DEBUG, "igid not in cookie, but need auth");
        mem_release(ctx->page);
        ctx->page = NULL;
      } else {
        if (!process_auth(igid, ctx->page)) {
          mem_release(ctx->page);
          ctx->page = NULL;
        }
      }
    }

    if (ctx->page != NULL) {
      send_page(ctx);
    } else {
      // not authorized: redirect to 403 page
      //pass_to_upstream(ctx);
      char buf[4096];
      tlog(DEBUG, "pass(no permission): %s", ctx->req.url);
      if (is_multi_keyword_domain(ctx->req.host)) {
	char kw[strlen(ctx->req.keyword)*5+1];
	http_escape(ctx->req.keyword, kw);
	sprintf(buf, "http://%s/%s%s", ctx->req.host, kw, cfg.page403);
      } else {
	sprintf(buf, "http://%s%s", ctx->req.host, cfg.page403);
      }
      send_redirect(ctx, buf);
    }
  }
}

static void
cache_process(gpointer data, gpointer user_data)
{
  req_ctx_t *ctx = data;
  if (ctx != NULL) {
    process_cache(&ctx->req, ctx->page);
    free(ctx);
  }
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
  cache = g_thread_pool_new(cache_process,NULL, 4, false, &error);
  if (fastp == NULL || slowp == NULL || cache == NULL) {
    perror("can not initialize thread pool!");
    exit(1);
  }
  poll_monitor_result();
}

void
page_handler(struct evhttp_request *req, void *arg)
{
  GError *error;
  req_ctx_t *ctx ;

  // enter from evhttpd
  if ((ctx = calloc(1, sizeof(req_ctx_t))) == NULL) {
    tlog(ERROR, "failed to create context: %s", req->uri);
    return;
  }
  if ((ctx->resp_buf = evbuffer_new()) == NULL) {
    tlog(ERROR, "failed to create response buffer for: %s", req->uri);
    return;
  }
  ctx->resp_code   = HTTP_OK;
  ctx->resp_reason = "OK";

  ctx->client_req  = req;
  ctx->arg  = arg;
  ctx->page = NULL;
  request_init(&ctx->req);
  tlog(DEBUG, "push %s to fast pool", req->uri);
  g_thread_pool_push(fastp, ctx, &error);
}

void
send_handler(struct evhttp_request *req, void *arg)
{
  GError *error;
  req_ctx_t *ctx = NULL;
  while(true) {
    ctx = send_list_pop();
    if (ctx == NULL) break;
    tlog(DEBUG, "send %s", ctx->client_req->uri);
    evhttp_send_reply(ctx->client_req,
                      ctx->resp_code,
                      ctx->resp_reason,
                      ctx->resp_buf);
    evbuffer_free(ctx->resp_buf);
    g_thread_pool_push(cache, ctx, &error);
  };
  evhttp_send_error(req, HTTP_NOCONTENT, "No Content");
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

void
status_handler(struct evhttp_request *req, void *arg)
{
  struct evbuffer *buf;
  if ((buf = evbuffer_new()) == NULL) {
    printf("failed to create response buffer");
  } else {
    char html[32768] = {'\0'};
    process_stat_html(html);
    evbuffer_add(buf, html, strlen(html));
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
  }
}

void
read_kw_handler(struct evhttp_request *req, void *arg)
{
  if (cfg.doamin_file != NULL)  read_domain(cfg.doamin_file);
  if (cfg.synonyms_file != NULL)read_synonyms(cfg.synonyms_file);
  evhttp_send_error(req, HTTP_NOCONTENT, "No Content");
}

void
page_save_handler(struct evhttp_request *req, void *arg)
{
  struct evbuffer *buf;
  if ((buf = evbuffer_new()) == NULL) {
    printf("failed to create response buffer");
  } else {
    uint64_t page_id=0, save_time=UNKNOWN_SAVE_TIME;
    char *pids = strstr(req->uri, "pid=");
    if (pids!=NULL && pids+4!='\0') {
      pids+=4; // strlen("pid="):4
      page_id = atoll(pids);
      save_time = page_save_time(page_id);
    }

    evbuffer_add_printf(buf, "uri=%s, page=%lld, save_time=%lld\n", req->uri, page_id, save_time);
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

  tlog(INFO, "fcache start.");

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
  wait_page_save_message();
  init_fcache();
  // keywords
  read_lock_init();
  if (cfg.doamin_file != NULL)  read_domain(cfg.doamin_file);
  if (cfg.synonyms_file != NULL)read_synonyms(cfg.synonyms_file);
  process_sticky();

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
  if (cfg.status_path  != NULL)
    evhttp_set_cb(httpd, cfg.status_path,  status_handler,  NULL);
  if (cfg.read_kw_path != NULL)
    evhttp_set_cb(httpd, cfg.read_kw_path,  read_kw_handler,  NULL);
  if (cfg.page_save_path != NULL)
    evhttp_set_cb(httpd, cfg.page_save_path,page_save_handler,NULL);

  evhttp_set_cb(httpd, send_start_path,  send_handler,  NULL);

  /* Set a callback for all other requests. */
  evhttp_set_gencb(httpd, page_handler, NULL);

  tlog(DEBUG, "start done: %s, %d", cfg.bind_addr, cfg.port);
  event_dispatch();
  /* Not reached in this code as it is now. */
  evhttp_free(httpd);

  return EXIT_SUCCESS;
}
