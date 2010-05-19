#include "be_read.h"

int16_t
read(stream_t *s)
{ // read 1 byte
  int16_t ret = -1;
  if (s->pos <= s->len) {
    ret = *(s->data+((s->pos)++));
  }
  return ret;
}

bool
readbool(stream_t *s)
{
  return read(s) != 0 ? true : false;
}

int8_t
read8(stream_t *s)
{
  return (int8_t)read(s);
}

int16_t
read16(stream_t *s)
{
  int16_t ch1 = read(s);
  int16_t ch2 = read(s);
  return (int16_t)((ch1 << 8) + (ch2 << 0));
}

int32_t
read32(stream_t *s)
{
  int16_t ch1 = read(s);
  int16_t ch2 = read(s);
  int16_t ch3 = read(s);
  int16_t ch4 = read(s);
  return (int32_t)((ch1 << 24) + (ch2 << 16) + (ch3 << 8) + (ch4 << 0));
}

int64_t
read64(stream_t *s)
{
  int64_t ch1 = read(s);
  int64_t ch2 = read(s);
  int64_t ch3 = read(s);
  int64_t ch4 = read(s);
  int16_t ch5 = read(s);
  int16_t ch6 = read(s);
  int16_t ch7 = read(s);
  int16_t ch8 = read(s);
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
  return (uint8_t) read(s);
}

uint16_t
readu16(stream_t *s)
{
  int16_t ch1 = read(s);
  int16_t ch2 = read(s);
  return (uint16_t)((ch1 << 8) + (ch2 << 0));
}

uint32_t
readu32(stream_t *s)
{
  int16_t ch1 = read(s);
  int16_t ch2 = read(s);
  int16_t ch3 = read(s);
  int16_t ch4 = read(s);
  return (uint32_t)((ch1 << 24) + (ch2 << 16) + (ch3 << 8) + (ch4 << 0));
}

uint64_t
readu64(stream_t *s)
{
  int64_t ch1 = read(s);
  int64_t ch2 = read(s);
  int64_t ch3 = read(s);
  int64_t ch4 = read(s);
  int16_t ch5 = read(s);
  int16_t ch6 = read(s);
  int16_t ch7 = read(s);
  int16_t ch8 = read(s);
  return (int64_t)((int64_t)(ch1 << 56) +
		   (int64_t)((ch2&255) << 48) +
		   (int64_t)((ch3&255) << 40) +
                   (int64_t)((ch4&255) << 32) +
		   (int64_t)((ch5&255) << 24) +
		   (int64_t)((ch6&255) << 16) +
                   (int64_t)((ch7&255) << 8)  +
		   (int64_t)((ch8&255) << 0));
}
