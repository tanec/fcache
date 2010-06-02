#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Config.h"
#include "fastcgi-api.h"
#include "event-api.h"
#include "log.h"
#include "util.h"
#include "sockutil.h"

#include "fcache.h"
#include "settings.h"
#include "process.h"
#include "read_file.h"
#include "zhongsou_keyword.h"

#define FCGI_LISTENSOCK_FILENO 0
#define FLAG_KEEP_CONN 1

typedef struct {
  struct event ev;
} server_t;

static void
exit_on_sig(const int sig)
{
  tlog(FATAL, "signal: received=%d", sig);
  exit(EXIT_SUCCESS);
}

/*************** request section ***************/
inline static fcgi_request_t *
request_get(uint16_t id) {
  fcgi_request_t *r;
  r = _requests[id];
  if (r == NULL) {
    r = (fcgi_request_t *)calloc(1, sizeof(fcgi_request_t));
    _requests[id] = r;
  }
  tlog(DEBUG, "** get req %p", r);
  return r;
}

inline static void
request_put(fcgi_request_t *r)
{
  tlog(DEBUG, "** put req %p", r);
  r->bev->cbarg = NULL;
  r->bev = NULL;
}

static inline bool
request_is_active(fcgi_request_t *r)
{
  return ((r != NULL) && (r->bev != NULL));
}

void
request_write(fcgi_request_t *r, const char *buf, uint16_t len, uint8_t tostdout)
{
  if (len == 0) return;
  if (!request_is_active(r)) return;

  FCGI_header_t h;
  FCGI_header_init(&h, tostdout ? TYPE_STDOUT : TYPE_STDERR, r->id, len);

  if (evbuffer_add(r->bev->output, (const void *)&h, sizeof(FCGI_header_t)) != -1)
    evbuffer_add(r->bev->output, (const void *)buf, len);

  // schedule write
  if (r->bev->enabled & EV_WRITE)
    bev_add(&r->bev->ev_write, r->bev->timeout_write);
}

void
request_end(fcgi_request_t *r, uint32_t appstatus, uint8_t protostatus)
{
  if (!request_is_active(r)) return;

  uint8_t buf[32]; // header + header + end_request_t
  uint8_t *p = buf;

  // Terminate the stdout and stderr stream, and send the end-request message.
  FCGI_header_init((FCGI_header_t *)p, TYPE_STDOUT, r->id, 0);
  p += sizeof(FCGI_header_t);
  FCGI_header_init((FCGI_header_t *)p, TYPE_STDERR, r->id, 0);
  p += sizeof(FCGI_header_t);
  FCGI_end_request_init((FCGI_end_request_t *)p, r->id, appstatus, protostatus);
  p += sizeof(FCGI_end_request_t);

  tlog(DEBUG, "sending END_REQUEST for id %d", r->id);

  bufferevent_write(r->bev, (const void *)buf, sizeof(buf));

  r->terminate = true;
}
/*************** request section end ***********/

/*************** handle  section ***************/
void
app_handle_beginrequest(fcgi_request_t *r)
{
  printf("app_handle_beginrequest %p\n", r);

  if (r->role != ROLE_RESPONDER) {
    request_write(r, "We can't handle any role but RESPONDER.", 39, 0);
    request_end(r, 1, PROTOST_UNKNOWN_ROLE);
    return;
  }

  // DIRECT
  static const char hello[] = "Content-type: text/plain\r\n\r\nHello world\n";
  request_write(r, hello, sizeof(hello)-1, 1);
  request_end(r, 0, PROTOST_REQUEST_COMPLETE);
}

void
app_handle_input(fcgi_request_t *r, uint16_t length)
{
  printf("app_handle_input %p -- %d bytes\n", r, length);
  // simply drain it for now
  evbuffer_drain(r->bev->input, length);
}

void
app_handle_requestaborted(fcgi_request_t *r)
{
  tlog(INFO, "app_handle_requestaborted %p\n", r);
}
/*************** handle  section end ***********/

/*************** process section ***************/
inline static void
process_begin_request(struct bufferevent *bev, uint16_t id, const FCGI_begin_request_t *br)
{
  fcgi_request_t *r;

  r = request_get(id);
  if ((r->bev != NULL) && (EVBUFFER_LENGTH(r->bev->input) != 0)) {
    tlog(WARN, "client sent already used req id %d -- skipping", id);
    bev_close(r->bev);
    return;
  }

  r->bev = bev;
  r->id = id;
  r->keepconn = (br->flags & FLAG_KEEP_CONN) == 1;
  r->role = (br->roleB1 << 8) + br->roleB0;
  r->stdin_eof = false;
  r->terminate = false;
  bev->cbarg = (void *)r;

  evbuffer_drain(bev->input, sizeof(FCGI_header_t)+sizeof(FCGI_begin_request_t));
}

inline static void
process_abort_request(fcgi_request_t *r)
{
  assert(r->bev != NULL);
  tlog(DEBUG, "request %p aborted by client", r);

  app_handle_requestaborted(r);

  r->terminate = 1; // can we trust fcache_writecb to be called?
}

inline static void
process_params(struct bufferevent *bev, uint16_t id, const uint8_t *buf, uint16_t len)
{
  fcgi_request_t *r;

  r = request_get(id);

  // Is this the last message to come? Then queue the request for the user.
  if (len == 0) {
    evbuffer_drain(bev->input, sizeof(FCGI_header_t));
    app_handle_beginrequest(r);
    return;
  }

  // Process message.
  uint8_t const * const bufend = buf + len;
  uint32_t name_len;
  uint32_t data_len;

  while(buf != bufend) {
    if (*buf >> 7 == 0) {
      name_len = *(buf++);
    } else {
      name_len = ((buf[0] & 0x7F) << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
      buf += 4;
    }

    if (*buf >> 7 == 0) {
      data_len = *(buf++);
    } else {
      data_len = ((buf[0] & 0x7F) << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
      buf += 4;
    }

    assert(buf + name_len + data_len <= bufend);

    // todo replace with actual adding to req:
    char k[255], v[8192];
    strncpy(k, (const char *)buf, name_len); k[name_len] = '\0';
    buf += name_len;
    strncpy(v, (const char *)buf, data_len); v[data_len] = '\0';
    buf += data_len;
    tlog(DEBUG, "fcache>> param>> '%s' => '%s'", k, v);
    // todo: req->second->params[name] = data;
  }

  evbuffer_drain(bev->input, sizeof(FCGI_header_t) + len);
}

inline static void
process_stdin(struct bufferevent *bev, uint16_t id, const uint8_t *buf, uint16_t len)
{
  fcgi_request_t *r;

  r = request_get(id);

  // left-over stdin on inactive request is drained and forgotten
  if (r->bev == NULL) {
    bev_drain(bev);
    return;
  }

  assert(r->bev != NULL);
  evbuffer_drain(bev->input, sizeof(FCGI_header_t));

  // Is this the last message to come? Then set the eof flag.
  // Otherwise, add the data to the buffer in the request structure.
  if (len == 0) {
    r->stdin_eof = true;
    return;
  }

  app_handle_input(r, len);
}

inline static void
process_unknown(struct bufferevent *bev, uint8_t type, uint16_t len)
{
  tlog(ERROR, "process_unknown(%p, %d, %d)", bev, type, len);
  FCGI_unknown_type_t msg;
  FCGI_unknown_type_init(&msg, type);
  bufferevent_write(bev, (const void *)&msg, sizeof(FCGI_unknown_type_t));
  evbuffer_drain(bev->input, sizeof(FCGI_header_t) + len);
}
/*************** process section end ***********/

/*************** callback section **************/
void
fcache_readcb(struct bufferevent *bev, fcgi_request_t *r)
{
  tlog(DEBUG, "fcache_readcb(%p, %p)", bev, r);

  while(EVBUFFER_LENGTH(bev->input) >= sizeof(FCGI_header_t)) {
    const FCGI_header_t *hp = (const FCGI_header_t *)EVBUFFER_DATA(bev->input);

    // Check whether our peer speaks the correct protocol version.
    if (hp->version != 1) {
      tlog(FATAL, "fcache: cannot handle protocol version %u", hp->version);
      bev_abort(bev);
      break;
    }

    // Check whether we have the whole message that follows the
    // headers in our buffer already. If not, we can't process it
    // yet.
    uint16_t msg_len = (hp->contentLengthB1 << 8) + hp->contentLengthB0;
    uint16_t msg_id = (hp->requestIdB1 << 8) + hp->requestIdB0;

    if (EVBUFFER_LENGTH(bev->input) < sizeof(FCGI_header_t) + msg_len + hp->paddingLength)
      return;

    // Process the message.
    tlog(DEBUG,
         "fcache>> received message: id: %d, bodylen: %d, padding: %d, type: %d",
         msg_id, msg_len, hp->paddingLength, (int)hp->type);

    switch (hp->type) {
    case TYPE_BEGIN_REQUEST:
      process_begin_request(bev, msg_id,
                            (const FCGI_begin_request_t *)(EVBUFFER_DATA(bev->input) + sizeof(FCGI_header_t)) );
      break;
    case TYPE_ABORT_REQUEST:
      process_abort_request(request_get(msg_id));
      break;
    case TYPE_PARAMS:
      process_params(bev, msg_id, (const uint8_t *)EVBUFFER_DATA(bev->input) + sizeof(FCGI_header_t), msg_len);
      break;
    case TYPE_STDIN:
      process_stdin(bev, msg_id, (const uint8_t *)EVBUFFER_DATA(bev->input) + sizeof(FCGI_header_t), msg_len);
      break;
      //case TYPE_END_REQUEST:
      //case TYPE_STDOUT:
      //case TYPE_STDERR:
      //case TYPE_DATA:
      //case TYPE_GET_VALUES:
      //case TYPE_GET_VALUES_RESULT:
      //case TYPE_UNKNOWN:
    default:
      process_unknown(bev, hp->type, msg_len);
    }/* switch(hp->type) */

    if (hp->paddingLength)
      evbuffer_drain(bev->input, hp->paddingLength);
  }
}

void
fcache_writecb(struct bufferevent *bev, fcgi_request_t *r)
{
  // Invoked if bev->output is drained or below the low watermark.
  tlog(DEBUG, "fcache_writecb(%p, %p)", bev, r);

  if (r != NULL && r->terminate) {
    bev_disable(r->bev);
    bev_drain(r->bev);
    bev_close(r->bev);
    request_put(r);
    if (r->keepconn == false) {
      tlog(ERROR, "PUT connection (r->keepconn == false, in fcache_writecb)");
    }
  }
}

void
fcache_errorcb(struct bufferevent *bev, short what, fcgi_request_t *r)
{
  if (what & EVBUFFER_EOF) {
    tlog(ERROR, "request %p EOF", r);
    process_abort_request(r);
  } else if (what & EVBUFFER_TIMEOUT)
    tlog(ERROR, "request %p timeout", r);
  else
    tlog(ERROR, "request %p error", r);

  bev_close(bev);
  if (r) request_put(r);
}
/*************** callback section end **********/

static const char *
sockaddr_host(const sau_t *sa)
{
#define SOCK_MAXADDRLEN 255
  static char *buf[SOCK_MAXADDRLEN+1];
  buf[0] = '\0';
  struct sockaddr_in *sk = (struct sockaddr_in *)sa;
  return inet_ntop(AF_INET, &(sk->sin_addr), (char *)buf, SOCK_MAXADDRLEN);
}

/*************** server section ****************/
const sau_t *
server_bind(server_t *server)
{
  static sau_t sa;
  if ((server->ev.ev_fd = bind_tcp(cfg.bind_addr, cfg.port, SOMAXCONN)) < 0)
    return NULL;
  return (const sau_t *)(&sa);
}

void
server_accept(int fd, short ev, server_t *server)
{
  struct bufferevent *bev;
  socklen_t saz;
  int on = 1, events, connfd;
  struct timeval *timeout;
  sau_t sa;

  saz = sizeof(sa);
  connfd = accept(fd, (struct sockaddr *)&sa, &saz);
  timeout = NULL;

  if (connfd < 0) {
    warn("accept failed");
    return;
  }

  // Disable Nagle -- better response times at the cost of more packets being sent.
  setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));
  // Set nonblocking
  AZ(ioctl(connfd, FIONBIO, (int *)&on));

  bev = bufferevent_new(connfd,
                        (evbuffercb)fcache_readcb,
                        (evbuffercb)fcache_writecb,
                        (everrorcb)fcache_errorcb,
                        NULL);

  events = EV_READ;
  if (bev->writecb)
    events |= EV_WRITE;
  bufferevent_enable(bev, events);

  tlog(DEBUG, "GET connection\n");
  tlog(DEBUG, "fcgi client %s connected on fd %d\n", sockaddr_host(&sa), connfd);
}

void
server_enable(server_t *server)
{
  int on = 1;
  AZ(ioctl(server->ev.ev_fd, FIONBIO, (int *)&on));
  event_set(&server->ev, server->ev.ev_fd, EV_READ|EV_PERSIST,
            (void (*)(int,short,void*))server_accept, (void *)server);
  event_add(&server->ev, NULL/* no timeout */);
}
/*************** server section end ************/

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

                           // debug
                           "r:"  /* test read a file */
                           ))) {
    switch (c) {
    case 'C':
        read_cfg(&cfg, optarg);
    case 'd':
      cfg.daemon = 1;

    case 'I':
      cfg.bind_addr = optarg;
      break;
    case 'p':
      cfg.port = (uint16_t)atoi(optarg);
      cfg.conn_type = tcp;
      break;
    case 's':
      cfg.socketpath = optarg;
      cfg.conn_type = domain_socket;
      break;

    case 'c':
      cfg.maxconns = atoi(optarg);
      break;

    case 'P':
      cfg.pid_file = optarg;
      break;

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

    case 'r':
      page_print(file_read_path(optarg));
      return 0;
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

  if (getuid() == 0 || geteuid() == 0) {
    printf("should not run as root. dangerious!\n");
    exit(1);
  }

  if (cfg.daemon) {
    if (sigignore(SIGHUP) == -1) {
      perror("Failed to ignore SIGHUP");
    }

    daemonize(1,1);
  }
  // files for log

  // Initialization
  process_init();
  // keywords
  if (cfg.doamin_file != NULL)  read_domain(cfg.doamin_file);
  if (cfg.synonyms_file != NULL)read_synonyms(cfg.synonyms_file);

  // initialize libraries
  event_init();
  memset((void *)_requests, 0, sizeof(_requests));

  server_t *server = calloc(1,sizeof(server_t));
  server->ev.ev_fd = FCGI_LISTENSOCK_FILENO;
  if (server_bind(server) <= 0) err(1, "server_bind");
  tlog(INFO, "listening on %s:%d", cfg.bind_addr, cfg.port);
  server_enable(server);

  // enter runloop
  event_dispatch();

  return EXIT_SUCCESS;
}
