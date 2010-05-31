#ifndef EVENTAPI_H
#define EVENTAPI_H

#include <sys/time.h>
#include <event.h>

/***************** bufferevent section *****************/
inline void bev_drain(struct bufferevent *);
inline void bev_disable(struct bufferevent *);
inline void bev_close(struct bufferevent *);
void bev_abort(struct bufferevent *);
inline int bev_add(struct event *, int);
/***************** bufferevent section end *************/

#endif // EVENTAPI_H
