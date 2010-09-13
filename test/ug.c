#include <stdio.h>
#include <signal.h>
#include "util.h"

int
main(int argc, char **argv)
{
  int i;
  sigignore(SIGPIPE);
  for (i=1; i<argc; i++) {
    tbuf r={0, NULL}, w={0, NULL};
    tbuf_read(&r, argv[i]);
    ext_gunzip(&w, r.data, r.len);
    printf("%6d: %s(%d) -->\n%s\n\n", getpid(), argv[i], r.len, w.data);
    tbuf_close(&r); tbuf_close(&w);
  }
  return 0;
}

