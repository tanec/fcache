#include <sys/socket.h>
#include <stddef.h>
#include "event-api.h"
#include "util.h"

#define BEV_FD(p) ((p)->ev_read.ev_fd)
#define BEV_FD_SET(p, v) ((p)->ev_read.ev_fd = (v))

/***************** bufferevent section *****************/
inline void
bev_drain(struct bufferevent *bev)
{
  evbuffer_drain(bev->input,  EVBUFFER_LENGTH(bev->input));
  evbuffer_drain(bev->output, EVBUFFER_LENGTH(bev->output));
}

inline void
bev_disable(struct bufferevent *bev)
{
  int events = 0;
  if (bev->readcb)  events |= EV_READ;
  if (bev->writecb) events |= EV_WRITE;
  bufferevent_disable(bev, events);
}

inline void
bev_close(struct bufferevent *bev)
{
  bev_disable(bev);
  close(BEV_FD(bev));
  BEV_FD_SET(bev, -1);
}

void
bev_abort(struct bufferevent *bev)
{
  struct linger lingeropt;
  lingeropt.l_onoff = 1;
  lingeropt.l_linger = 0;
  AZ(setsockopt(BEV_FD(bev), SOL_SOCKET, SO_LINGER, (char *)&lingeropt, sizeof(lingeropt)));
  shutdown(BEV_FD(bev), SHUT_RDWR);
  bev_drain(bev);
  bev_close(bev);
}

inline int
bev_add(struct event *ev, int timeout)
{
  struct timeval tv, *ptv = NULL;
  if (timeout) {
    evutil_timerclear(&tv);
    tv.tv_sec = timeout;
    ptv = &tv;
  }
  return event_add(ev, ptv);
}
/***************** bufferevent section end *************/
