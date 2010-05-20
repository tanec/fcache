#ifndef BE_READ_H
#define BE_READ_H
/* big endian: java stream, network */
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  size_t pos, len;
  uint8_t* data;
} stream_t;

bool    readbool(stream_t *);
int8_t  read8(stream_t *);
int16_t read16(stream_t *);
int32_t read32(stream_t *);
int64_t read64(stream_t *);

uint8_t  readu8(stream_t *);
uint16_t readu16(stream_t *);
uint32_t readu32(stream_t *);
uint64_t readu64(stream_t *);

char *readstr(stream_t *);

#endif // BE_READ_H
