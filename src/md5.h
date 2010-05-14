#ifndef MD5_H
#define MD5_H

#include <stdint.h>

typedef uint8_t  md5_byte_t; /* 8-bit byte */
typedef uint32_t md5_word_t; /* 32-bit word */

typedef struct {
  md5_word_t count[2];        /* message length in bits, lsw first */
  md5_word_t abcd[4];         /* digest buffer */
  md5_byte_t buf[64];         /* accumulate block */
} md5_state_t;

void md5_init(md5_state_t *);
void md5_append(md5_state_t *, const md5_byte_t *, int);
void md5_finish(md5_state_t *state, md5_byte_t digest[16]);

#endif // MD5_H
