#ifndef FASTCGIAPI_H
#define FASTCGIAPI_H

#include <stdint.h>

typedef struct {
  uint8_t version;
  uint8_t type;
  uint8_t requestIdB1;
  uint8_t requestIdB0;
  uint8_t contentLengthB1;
  uint8_t contentLengthB0;
  uint8_t paddingLength;
  uint8_t reserved;
} FCGI_header_t; // 8

typedef struct {
  uint8_t roleB1;
  uint8_t roleB0;
  uint8_t flags;
  uint8_t reserved[5];
} FCGI_begin_request_t; // 8

typedef struct {
  FCGI_header_t header;
  uint8_t appStatusB3;
  uint8_t appStatusB2;
  uint8_t appStatusB1;
  uint8_t appStatusB0;
  uint8_t protocolStatus;
  uint8_t reserved[3];
} FCGI_end_request_t; // 16

typedef struct {
  FCGI_header_t header;
  uint8_t type;
  uint8_t reserved[7];
} FCGI_unknown_type_t; // 16

enum FCGI_header_type { // header.type
  TYPE_BEGIN_REQUEST     = 1,
  TYPE_ABORT_REQUEST     = 2,
  TYPE_END_REQUEST       = 3,
  TYPE_PARAMS            = 4,
  TYPE_STDIN             = 5,
  TYPE_STDOUT            = 6,
  TYPE_STDERR            = 7,
  TYPE_DATA              = 8,
  TYPE_GET_VALUES        = 9,
  TYPE_GET_VALUES_RESULT = 10,
  TYPE_UNKNOWN           = 11
};

inline void FCGI_header_init(FCGI_header_t *, uint8_t, uint16_t, uint16_t);
inline void FCGI_end_request_init(FCGI_end_request_t *, uint16_t, uint32_t, uint8_t);
inline void FCGI_unknown_type_init(FCGI_unknown_type_t *, uint8_t);


#endif // FASTCGIAPI_H
