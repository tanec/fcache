#ifndef READER_H
#define READER_H

#include <stdint.h>
#include "md5.h"

extern size_t total_pages;

typedef struct {
  uint8_t  version;      // 静态页版本格式
  uint8_t  valid;        // 是否有效
  uint64_t time_expire;  // 过期时间毫秒
  uint64_t time_create;  // 生成时间毫秒
  uint64_t time_dead;    // 逾期时间毫秒，超过此时间不返回静态页
  uint64_t page_no;      // 页号
  uint8_t  type;         // 站类型  0 私有 1共享 >1 会员
  //    uint32_t keyword_len;  // 关键词长度
  //    byte[]   keyword;      // 关键词
  char *keyword;
  //    uint32_t ig_len;       // ig号长度
  //    byte[]   ig;
  char *ig;
  uint32_t auth_type;    // 鉴权方式  0 不鉴权，1特殊页面鉴权，2php 特殊页面鉴权3，会员鉴权
  //    uint32_t param_len;    // 额外参数的长度
  //    byte[]   param;        // 额外参数{PageAuth:{sid:,appid,authid:},GroupAuth:{sid,appid,authid}}
  char *param;
} page_head_t;

typedef enum {
  AUTH_NO     = 0,
  AUTH_PAGE   = 1,
  AUTH_PHP    = 2,
  AUTH_MEMBER = 3
} auth_type_t;

typedef struct {
  page_head_t head;
  size_t page_len;
  size_t body_len;
  void *body; // 文件内容;
} page_t;

void page_print(page_t *);

#endif // READER_H
