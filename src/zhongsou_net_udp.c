#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "zhongsou_net_udp.h"
#include "settings.h"
#include "thread.h"
#include "log.h"

/*
UDP 协议
byte 2
byte 0
byte 1
byte 2
以上字节是校验
int length json 串的长度
{Type:1,URL:{OrignalURL:"",ApacheURL:{}}}

里边的ApacheURL是静态页里的json串，原封给我发过来就可以
 */

void
udp_notify_expire(request_t *req, page_t *page)
{
  const char *url=req->url, *param=page->head.param;
  size_t buf_len = 64+strlen(url)+strlen(param), real_len, be_len;
  char buf[buf_len], sendbuf[8+buf_len], *p;

  struct sockaddr_in si_other;
  int i, s, slen=sizeof(si_other);

  for (i=0; i<cfg.udp_notify.num; i++) {
    server_t *svr = next_server_in_group(&cfg.udp_notify);
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) continue;
    memset((char *) &si_other, 0, slen);
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(svr->port);
    if (inet_aton(svr->host, &si_other.sin_addr)==0) {
      fprintf(stderr, "inet_aton() failed\n");
      continue;
    }

    // prepare data
    memset(buf, 0, buf_len);
    snprintf(buf, buf_len,
             "{Type:1,URL:{OrignalURL:\"%s\",ApacheURL:%s}}",
             url, param);
    real_len = strlen(buf);
    sendbuf[0] = 2;
    sendbuf[1] = 0;
    sendbuf[2] = 1;
    sendbuf[3] = 2;
    be_len = htonl(real_len);
    p = (char *)&be_len;
    sendbuf[4] = *p;
    sendbuf[5] = *(p+1);
    sendbuf[6] = *(p+2);
    sendbuf[7] = *(p+3);
    memset(sendbuf+8, 0, buf_len);
    memcpy(sendbuf+8, buf, real_len);

    // send it
    ssize_t n, pos=0;
    do {
      n=sendto(s, sendbuf+pos, real_len+8-pos, MSG_DONTWAIT,
               (struct sockaddr*)&si_other, slen);
      if (n < 0) {
         tlog(ERROR, "send udp expire notify failed!");
         break;
      }

      pos += n;
    } while(pos < real_len);
    close(s);
  }
}

#define BUFLEN 1200
static int listen_socket;

void *
handle_expire_request(void *args)
{
  char buf[BUFLEN];
  struct sockaddr_in si_other;
  int i, slen=sizeof(si_other);
  ssize_t len;

  tlog(DEBUG, "udp server @%d start", cfg.udp_server.port);
  while(true)
  if ((len=recvfrom(listen_socket, buf, BUFLEN, 0,
                    (struct sockaddr *)&si_other, &slen))!=-1) {
    tlog(DEBUG, "Received packet from %s:%d\nData(%d): %s\n\n",
           inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), len, buf);
    if (len>=20 && args!=NULL
        && buf[0]==2
        && buf[1]==0
        && buf[2]==1
        && buf[3]==2) {
      md5_digest_t dig;
      for (i=0; i<16; i++) dig.digest[i] = buf[i+4];

      expire_cb cb = (expire_cb)args;
      cb(&dig);
    }
  }
  tlog(ERROR, "udp server @%d shutdown", cfg.udp_server.port);
}

void
udp_listen_expire(expire_cb cb)
{
  struct sockaddr_in si_me;

  if ((listen_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    perror("udp listen: socket");

  memset((char *) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(cfg.udp_server.port);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(listen_socket, (const struct sockaddr *)&si_me, sizeof(si_me))==-1)
    perror("udp listen: bind");

  create_worker(handle_expire_request, cb);
}
