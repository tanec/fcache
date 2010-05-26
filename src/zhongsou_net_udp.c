#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "zhongsou_net_udp.h"
#include "settings.h"

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
  char *url=req->url, *param=page->head.param;
  size_t buf_len = 64+strlen(url)+strlen(param), real_len, be_len;
  char buf[buf_len], sendbuf[8+buf_len], *p;

  struct sockaddr_in si_other;
  int s, slen=sizeof(si_other);

  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) return;
  memset((char *) &si_other, 0, slen);
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(cfg.udp_notify_port);
  if (inet_aton(cfg.udp_notify_host, &si_other.sin_addr)==0) {
    fprintf(stderr, "inet_aton() failed\n");
    return;
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
  if (sendto(s, sendbuf, real_len, MSG_DONTWAIT,
             (struct sockaddr*)&si_other, slen) == -1) {
      //TODO
  }
  close(s);
}

void
udp_listen_expire(void)
{

}
