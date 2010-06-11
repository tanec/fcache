#include <sys/queue.h>
#include <event.h>
#include <evhttp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <string.h>

void
extract_cookie(struct evhttp_request *req)
{
  char s[8192];
#define d(i, f)           \
  do {                    \
      memset(s, 0, 8192); \
      strcat(s, #i);      \
      strcat(s, " = ");   \
      strcat(s, #f);      \
      strcat(s, "\n");    \
      printf(s, req->i);  \
    }while(0)
  d(flags, %d);
  d(remote_host, %s);
  d(remote_port, %d);
  d(kind, %d);
  d(type, %d);
  d(uri, %s);
  d(major, %d);
  d(minor, %d);
  d(response_code, %d);
  d(response_code_line, %s);

  printf("==> %d, %s\n", req->input_buffer->totallen, req->input_buffer->buffer);
  printf("==> %d, %s\n", req->output_buffer->totallen, req->output_buffer->buffer);


  struct evkeyvalq *headers = req->input_headers;
  struct evkeyval *h = headers->tqh_first;
  TAILQ_FOREACH(h, headers, next) {
    printf("h==> k=%s, v=%s\n", h->key, h->value);
  }

}

void
extract_req(struct evhttp_request *req)
{
  extract_cookie(req);
}

void
copycb(struct evhttp_request *req, void *arg)
{
  struct evbuffer *buf;
  extract_req(req);
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
  extract_req(req);
  if ((buf = evbuffer_new()) == NULL) {
    printf("failed to create response buffer");
  } else {
    char *ht="xxxxxxxxxxxx";
    printf("arg=%p\n", arg);
    evbuffer_add_printf(buf, "ht=%s\n", ht);
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
  }
}

void
proxycb(struct evhttp_request *req, void *arg)
{
  struct evhttp_connection *evcon;
  struct evbuffer *buf;
  extract_req(req);

  if ((evcon=0)==NULL) {

  }
  if ((buf = evbuffer_new()) == NULL) {
    printf("failed to create response buffer");
  } else {
    char *ht="xxxxxxxxxxxx";
    printf("arg=%p\n", arg);
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

  if (sigignore(SIGPIPE) == -1) {
    perror("failed to ignore SIGPIPE; sigaction");
    exit(EXIT_FAILURE);
  }

  event_init();
  httpd = evhttp_start("0.0.0.0", atoi(argv[1]));
  if (httpd == NULL) {
    perror("evhttpd failed!");
    exit(1);
  }

  /* Set a callback for requests to "/specific". */
  evhttp_set_cb(httpd, "/cgi/c", copycb, NULL);
  evhttp_set_cb(httpd, "/cgi/p", proxycb, NULL);

  /* Set a callback for all other requests. */
  evhttp_set_gencb(httpd, gencb, NULL);

  event_dispatch();
  /* Not reached in this code as it is now. */
  evhttp_free(httpd);

  return EXIT_SUCCESS;
}
