#include <stdio.h>
#include <string.h>

#include "read_file.h"
#include "be_read.h"
#include "util.h"
#include "smalloc.h"
#include "settings.h"

/*
头格式       编码gbk
Int 头大小
Int 内容大小
byte 静态页版本格式
byte 是否有效
long 过期时间毫秒
long 生成时间毫秒
long 页号
byte 站类型  0 私有 1共享 >1 会员
int  关键词长度
byte[] 关键词
int ig号长度
byte[] ig
Int  鉴权方式  0 不鉴权，1特殊页面鉴权，2php 特殊页面鉴权3，会员鉴权
int  额外参数的长度
byte[] 额外参数{PageAuth:{sid:,appid,authid:},GroupAuth:{sid,appid,authid}}
byte[] 文件内容
*/
page_t *
file_read_path(char *path)
{
  mmap_array_t mt;

  if (mmap_read(&mt, path)) {
    char *strpos;
    uint32_t strlen;
    stream_t s = {0, mt.len, mt.data};
    uint32_t hlen = readu32(&s);
    uint32_t blen = readu32(&s);

    /* |<-- page_t -->|<-- body data -->|<-- head strings -->| */
    /* about 32 bytes in head is not for strings */
    page_t *page = (page_t*)smalloc(sizeof(page_t)+blen+hlen-32);
    page->level = 0;
    page->body_len = blen;
    page->body = (char*)page+sizeof(page_t);
    memcpy(page->body, mt.data+2*sizeof(uint32_t)+hlen, page->body_len);
    strpos = (char*)page->body + blen;

    page_head_t *head = (page_head_t *)page;
    head->version = readu8(&s);
    head->valid = readu8(&s);
    head->time_expire = readu64(&s);
    head->time_create = readu64(&s);
    head->page_no = readu64(&s);
    head->type = readu8(&s);

    strlen = readu32(&s);
    readarr(&s, strpos, strlen);
    *(strpos+strlen) = 0;
    head->keyword = strpos;
    strpos += strlen+1;

    strlen = readu32(&s);
    readarr(&s, strpos, strlen);
    *(strpos+strlen) = 0;
    head->ig = strpos;
    strpos += strlen+1;

    head->auth_type = readu32(&s);

    strlen = readu32(&s);
    readarr(&s, strpos, strlen);
    *(strpos+strlen) = 0;
    head->param = strpos;
    strpos += strlen+1;

    mmap_close(&mt);
    return page;
  }
  return NULL;
}

page_t *
file_get(request_t *req)
{
  char path[71 + strlen(cfg.base_dir)]; //2*len(md5)+len('/'s)+'\0' = 2*32+6+1

  md5_dir(req);
  md5_file(req);
  sprintf(path,
          "%s/%02x%02x/%02x%02x/%02x%02x/%02x%02x/%02x%02x%02x%02x%02x%02x%02x%02x/%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\0",
	  cfg.base_dir,
          (*req).dig_dir.digest[0], (*req).dig_dir.digest[1], (*req).dig_dir.digest[2], (*req).dig_dir.digest[3],
          (*req).dig_dir.digest[4], (*req).dig_dir.digest[5], (*req).dig_dir.digest[6], (*req).dig_dir.digest[7],
          (*req).dig_dir.digest[8], (*req).dig_dir.digest[9], (*req).dig_dir.digest[10], (*req).dig_dir.digest[11],
          (*req).dig_dir.digest[12], (*req).dig_dir.digest[13], (*req).dig_dir.digest[14], (*req).dig_dir.digest[15],
          (*req).dig_file.digest[0], (*req).dig_file.digest[1], (*req).dig_file.digest[2], (*req).dig_file.digest[3],
          (*req).dig_file.digest[4], (*req).dig_file.digest[5], (*req).dig_file.digest[6], (*req).dig_file.digest[7],
          (*req).dig_file.digest[8], (*req).dig_file.digest[9], (*req).dig_file.digest[10], (*req).dig_file.digest[11],
          (*req).dig_file.digest[12], (*req).dig_file.digest[13], (*req).dig_file.digest[14], (*req).dig_file.digest[15]);

  return file_read_path(path);
}
