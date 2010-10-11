#include <stdio.h>

#include "util.h"

int
main(int argc, char **argv)
{
  if (argc == 2) {
    tbuf f = {0, NULL}, out={0, NULL};
    tbuf_read(&f, argv[1]);
    printf("read %d bytes from %s\n", f.len, argv[1]);
    zlib_gunzip(&out, f.data, f.len);
    if (out.data != NULL) printf("%s\n", out.data);
  }
  return 0;
}
