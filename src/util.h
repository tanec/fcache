#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <assert.h>

#ifndef ENABLE_TRACE
#define TRACE(...) do { } while (0)
#define ASSERT(x) do { } while (0)
#else
#define TRACE(flag, format, v1, v2) lwt_trace(flag, format, (size_t)(v1), (size_t)(v2))
#define ASSERT(x) do { if (!(x)) { lwt_halt(); assert(!#x); } } while (0)
#endif

#define EXPECT_TRUE(x)      __builtin_expect(!!(x), 1)
#define EXPECT_FALSE(x)     __builtin_expect(!!(x), 0)

#define SYNC_ADD(addr,n)          __sync_add_and_fetch(addr,n)
#define SYNC_SUB(addr,n)          __sync_sub_and_fetch(addr,n)
#define SYNC_CAS(addr,old,x)      __sync_val_compare_and_swap(addr,old,x)
#define SYNC_SWAP(addr,x)         __sync_lock_test_and_set(addr,x)
#define SYNC_FETCH_AND_OR(addr,x) __sync_fetch_and_or(addr,x)

#define TAG_VALUE(v, tag) ((v) |  tag)
#define IS_TAGGED(v, tag) ((v) &  tag)
#define STRIP_TAG(v, tag) ((v) & ~tag)

#define AZ(foo) assert((foo) == 0)
#define AN(foo) assert((foo) != 0)

typedef struct {
  off_t len;
  char *data;
} tbuf;

pid_t daemonize(int, int);

bool tbuf_read(tbuf *, const char *);
void tbuf_close(tbuf *);

// 1s = 1000 millisecond = 10^6microsecond
uint64_t current_time_millis(void);
uint64_t current_time_micros(void);

bool tcp_read(tbuf *, const char *, uint16_t, char *);

//string
void strtolower(char *);

bool ext_gunzip(tbuf *, const void*, size_t);
bool zlib_gunzip(tbuf *, const void*, size_t);

#ifdef __cplusplus
}
#endif
#endif // UTIL_H
