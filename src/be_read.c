#include <string.h>
#include "be_read.h"

uint8_t
readbyte(stream_t *s)
{ // read 1 byte
  uint8_t ret = 0;
  if (s->pos <= s->len) {
    ret = *(s->data+((s->pos)++));
  }
  return ret;
}

void
readarr(stream_t *s, char *dest, size_t size)
{
  memcpy(dest, s->data+s->pos, size);
  s->pos += size;
}

bool
readbool(stream_t *s)
{
  return readbyte(s) != 0 ? true : false;
}

int8_t
read8(stream_t *s)
{
  return (int8_t)readbyte(s);
}

int16_t
read16(stream_t *s)
{
  int16_t ch1 = readbyte(s);
  int16_t ch2 = readbyte(s);
  return (int16_t)((ch1 << 8) + (ch2 << 0));
}

int32_t
read32(stream_t *s)
{
  int32_t ch1 = readbyte(s);
  int32_t ch2 = readbyte(s);
  int32_t ch3 = readbyte(s);
  int32_t ch4 = readbyte(s);
  return (int32_t)((ch1 << 24) + (ch2 << 16) + (ch3 << 8) + (ch4 << 0));
}

int64_t
read64(stream_t *s)
{
  int64_t ch1 = readbyte(s);
  int64_t ch2 = readbyte(s);
  int64_t ch3 = readbyte(s);
  int64_t ch4 = readbyte(s);
  int64_t ch5 = readbyte(s);
  int64_t ch6 = readbyte(s);
  int64_t ch7 = readbyte(s);
  int64_t ch8 = readbyte(s);
  return (int64_t)((int64_t)(ch1 << 56) +
                   (int64_t)((ch2&255) << 48) +
                   (int64_t)((ch3&255) << 40) +
                   (int64_t)((ch4 &255)<< 32)+
                   (int64_t)((ch5&255) << 24) +
                   (int64_t)((ch6&255) << 16) +
                   (int64_t)((ch7&255) << 8) +
                   (int64_t)((ch8&255) << 0));
}

uint8_t
readu8(stream_t *s)
{
  return (uint8_t) readbyte(s);
}

uint16_t
readu16(stream_t *s)
{
  uint16_t ch1 = readbyte(s);
  uint16_t ch2 = readbyte(s);
  return (uint16_t)((ch1 << 8) + (ch2 << 0));
}

uint32_t
readu32(stream_t *s)
{
  uint32_t ch1 = readbyte(s);
  uint32_t ch2 = readbyte(s);
  uint32_t ch3 = readbyte(s);
  uint32_t ch4 = readbyte(s);
  return (uint32_t)((ch1 << 24) + (ch2 << 16) + (ch3 << 8) + (ch4 << 0));
}

uint64_t
readu64(stream_t *s)
{
  uint64_t ch1 = readbyte(s);
  uint64_t ch2 = readbyte(s);
  uint64_t ch3 = readbyte(s);
  uint64_t ch4 = readbyte(s);
  uint64_t ch5 = readbyte(s);
  uint64_t ch6 = readbyte(s);
  uint64_t ch7 = readbyte(s);
  uint64_t ch8 = readbyte(s);
  return (uint64_t)((uint64_t)(ch1 << 56) +
                   (uint64_t)((ch2&255) << 48) +
                   (uint64_t)((ch3&255) << 40) +
                   (uint64_t)((ch4&255) << 32) +
                   (uint64_t)((ch5&255) << 24) +
                   (uint64_t)((ch6&255) << 16) +
                   (uint64_t)((ch7&255) << 8)  +
                   (uint64_t)((ch8&255) << 0));
}
