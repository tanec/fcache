#include <stdio.h>
#include "reader.h"


void *
page_print(page_t *page)
{
  if (page == NULL) {
    printf("page = NULL\n");
    return;
  }
  printf("<== page@%llx start:\n", page);
  printf("  head.version=%d\n", page->head.version);
  printf("  head.valid=%d\n", page->head.valid);
  printf("  head.time_expire=%lld\n", page->head.time_expire);
  printf("  head.time_create=%lld\n", page->head.time_create);
  printf("  head.page_no=%lld\n", page->head.page_no);
  printf("  head.type=%d\n", page->head.type);
  printf("  head.keyword=%s\n", page->head.keyword);
  printf("  head.ig=%s\n", page->head.ig);
  printf("  head.auth_type=%d\n", page->head.auth_type);
  printf("  head.param=%s\n", page->head.param);
  printf("<== page@%llx end.\n", page);
}
