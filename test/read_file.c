#include <stdio.h>

#include "reader.h"
#include "read_file.h"

int
main(int argc, char **argv)
{
  int i;
  for (i=1; i<argc; i++) {
    printf("=== read: %s ===\n", argv[i]);
    page_print(file_read_path(argv[i]));
  }
  return 0;
}
