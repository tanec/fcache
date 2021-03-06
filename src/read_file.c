#include <stdio.h>
#include <string.h>

#include "read_file.h"
#include "be_read.h"
#include "util.h"
#include "smalloc.h"
#include "settings.h"
#include "log.h"

extern size_t total_pages = 0;
/*
头格式       编码gbk
Int 头大小
Int 内容大小
byte 静态页版本格式 0 or 1, 1:deadtime
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
  tbuf mt = {0, NULL};
  
  if (tbuf_read(&mt, path)) {
    char *strpos;
    uint32_t strlen;
    stream_t s = {0, mt.len, (uint8_t*)mt.data};
    size_t min = 2 * sizeof(uint32_t);
    if (mt.len < min) {
      tbuf_close(&mt);
      return NULL;
    }
    uint32_t hlen = readu32(&s);
    uint32_t blen = readu32(&s);
    if (mt.len != min+hlen+blen) {
      tbuf_close(&mt);
      return NULL;
    }

    /* |<-- page_t -->|<-- body data -->|<-- head strings -->| */
    /* about 32 bytes in head is not for strings */
    size_t plen = sizeof(page_t)+blen+hlen-32;
    page_t *page = (page_t*)smalloc(plen);
    page->page_len = plen;
    page->body_len = blen;
    page->body = (char*)page+sizeof(page_t);
    memcpy(page->body, mt.data+2*sizeof(uint32_t)+hlen, page->body_len);
    strpos = (char*)page->body + blen;

    page_head_t *head = &page->head;
    head->version = readu8(&s);
    switch(head->version) {
    case 0:
      head->valid = readu8(&s);
      head->time_expire = readu64(&s);
      head->time_create = readu64(&s);
      head->time_dead   = head->time_expire+(uint64_t)24*3600*1000;
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
      break;
    case 1:
      head->valid = readu8(&s);
      head->time_expire = readu64(&s);
      head->time_create = readu64(&s);
      head->time_dead   = readu64(&s);
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
      break;
    }

    tbuf_close(&mt);
    SYNC_ADD(&total_pages, 1);
    return page;
  }
  tbuf_close(&mt);
  return NULL;
}

void
file_path(char *path, md5_digest_t *dir, md5_digest_t *file)
{
  sprintf(path,
          "%s/%02x%02x/%02x%02x/%02x%02x/%02x%02x/%02x%02x%02x%02x%02x%02x%02x%02x/%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
	  cfg.base_dir,
          dir->digest[0], dir->digest[1], dir->digest[2], dir->digest[3],
          dir->digest[4], dir->digest[5], dir->digest[6], dir->digest[7],
          dir->digest[8], dir->digest[9], dir->digest[10], dir->digest[11],
          dir->digest[12], dir->digest[13], dir->digest[14], dir->digest[15],
          file->digest[0], file->digest[1], file->digest[2], file->digest[3],
          file->digest[4], file->digest[5], file->digest[6], file->digest[7],
          file->digest[8], file->digest[9], file->digest[10], file->digest[11],
          file->digest[12], file->digest[13], file->digest[14], file->digest[15]);
}
page_t *
file_get(md5_digest_t *dir, md5_digest_t *file)
{
  char path[71 + strlen(cfg.base_dir)]; //2*len(md5)+len('/')s+'\0' = 2*32+6+1
  file_path(path, dir, file);
  tlog(DEBUG, "file_read_path(%s)", path);
  return file_read_path(path);
}
