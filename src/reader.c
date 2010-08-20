#include <stdio.h>
#include "reader.h"


void
page_print(page_t *page)
{
  if (page == NULL) {
    printf("page = NULL\n");
    return;
  }
  printf("<== page@%p start:\n", page);
  printf("  head.version=%d\n", page->head.version);
  printf("  head.valid=%d\n", page->head.valid);
  printf("  head.time_expire=%lld(%llx)\n", page->head.time_expire, page->head.time_expire);
  printf("  head.time_create=%lld(%llx)\n", page->head.time_create, page->head.time_create);
  printf("  head.page_no=%lld(%llx)\n", page->head.page_no, page->head.page_no);
  printf("  head.type=%d\n", page->head.type);
  printf("  head.keyword=%s\n", page->head.keyword);
  printf("  head.ig=%s\n", page->head.ig);
  printf("  head.auth_type=%d\n", page->head.auth_type);
  printf("  head.param=%s\n", page->head.param);

  printf("  page.body_len=%d(%04x)\n  page.body:\n", page->body_len, page->body_len);
  unsigned char *b = (unsigned char *)page->body;
  int i, n=128;
  for(i=1; i<=n; i++){
      printf("%02x", *(b+i-1));
      if(i%32==0) { printf("\n"); continue; }
      if(i>0 && i%2==0) printf(" ");
  }
  printf("...\n");
  for(i=1; i<=n; i++){
      printf("%02x", *(b+page->body_len-n+i-1));
      if(i%32==0) { printf("\n"); continue; }
      if(i%2==0) printf(" ");
  }
  printf("<== page@%p end.\n", page);
}
