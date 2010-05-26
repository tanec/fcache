#ifndef ZHONGSOU_NET_UDP_H
#define ZHONGSOU_NET_UDP_H

#include "process.h"
#include "reader.h"

void udp_notify_expire(request_t *, page_t *);
void udp_listen_expire(void);

#endif // ZHONGSOU_NET_UDP_H
