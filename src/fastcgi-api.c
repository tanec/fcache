#include <string.h>
#include "fastcgi-api.h"

inline void
header_init(header_t *self, uint8_t t, uint16_t id, uint16_t len)
{
  self->version = '\1';
  self->type = t;
  self->requestIdB1 = id >> 8;
  self->requestIdB0 = id & 0xff;
  self->contentLengthB1 = len >> 8;
  self->contentLengthB0 = len & 0xff;
  self->paddingLength = '\0';
  self->reserved = '\0';
}

inline void
end_request_init(end_request_t *self, uint16_t id, uint32_t ast, uint8_t protostatus)
{
  header_init((header_t *)self, TYPE_END_REQUEST, id, sizeof(end_request_t)-sizeof(header_t));
  self->appStatusB3 = (ast >> 24) & 0xff;
  self->appStatusB3 = (ast >> 16) & 0xff;
  self->appStatusB3 = (ast >> 8) & 0xff;
  self->appStatusB3 = ast & 0xff;
  self->protocolStatus = protostatus;
  memset(self->reserved, 0, sizeof(self->reserved));
}

inline void
unknown_type_init(unknown_type_t *self, uint8_t unknown_type)
{
  header_init((header_t *)self, TYPE_UNKNOWN, 0, sizeof(unknown_type_t)-sizeof(header_t));
  self->type = unknown_type;
  memset(self->reserved, 0, sizeof(self->reserved));
}
