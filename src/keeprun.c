#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "util.h"

static pid_t
t_exec(char **argv, int verbose)
{
  pid_t pid;

  if (verbose) {
    int i;
    printf("invoke ...\n");
    for (i=0;; i++) {
      if (argv[i] == NULL) break;
      printf("  argv[%d] = %s\n", i, argv[i]);
    }
  }

  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "fork() < 0");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    // argv must be  terminated  by  a  NULL pointer.
    execvp(argv[0], argv);
  }

  return pid;
}

int
main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s <cmd> [cmd_args]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int i, status;
  char *args[argc];

  for(i=1; i<argc; i++) args[i-1] = argv[i];
  args[argc-1] = NULL;
  daemonize(0, 1);

  if ((t_exec(args, 0)) < 0) {
    fprintf(stderr, "first invoke failed.\n");
    exit(EXIT_FAILURE);
  }

  while(1) {
    if (wait(&status) < 0) {
      fprintf(stderr, "wait failed.\n");
      exit(EXIT_FAILURE);
    } else {
      t_exec(args, 1);
    }
  }
  exit(EXIT_FAILURE);
}
