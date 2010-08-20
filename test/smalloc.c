#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "smalloc.h"
#include "util.h"

int
main(int argc, char**argv)
{
  int i,j,n, cn=70000,
    count = atoi(argv[1]),
    size = atoi(argv[2]);
  
  void *ps[cn], *p=NULL;

  uint64_t tm = 0, now=0;
  for (i=0; i<count; i++) {
    now = current_time_millis();
    for(j=0; j<cn; j++) {
      ps[j] = NULL;
      if(j>99 && p!=NULL) {sfree(p); p=NULL;}
      n = 4+now%size;
      ps[j] = smalloc(n);
      memset(ps[j], 4, n);
      if (p==NULL && now%13==0) p = ps[j];
    }
    usleep(1000);
    if (now - tm > 1000) {
      printf("alloced: %lu bytes\n", smalloc_used_memory());
      tm = now;
    }
    for(j=0; j<cn; j++) {
      if(ps[j]!=p) sfree(ps[j]);
    }
  }
  return EXIT_SUCCESS;
}
