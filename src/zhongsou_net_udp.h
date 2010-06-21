#ifndef ZHONGSOU_NET_UDP_H
#define ZHONGSOU_NET_UDP_H

#include "process.h"
#include "reader.h"

typedef void (*expire_cb) (md5_digest_t *);

void udp_notify_expire(request_t *, page_t *);
void udp_listen_expire(expire_cb);

#endif // ZHONGSOU_NET_UDP_H
