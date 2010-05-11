#include "Config.h"
#include "common.h"
#include "fastcgi-api.h"
#include "event-api.h"
#include "curl-api.h"
#include "log.h"

void
die(int code, char* msg)
{
  fprintf(stderr, "exit(%d): %s\n", code, msg);
  exit(code);
}

void
config(char* file_name)
{
  
}

int
main(int argc, char**argv)
{
  pid_t pid, sid;
  
  pid = fork();
  if (pid < 0) {
    die(EXIT_FAILURE, "fork() < 0");
  } else if (pid > 0) {
    exit(EXIT_SUCCESS);
  }
  // prepare daemon
  umask(0);
  // files for log
  
  sid = setsid();
  if (sid < 0) {
    die(EXIT_FAILURE, "setsid() < 0");
  }
  if ((chdir("/")) < 0) {
    die(EXIT_FAILURE, "chdir(char*) < 0");
  }
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  
  // Initialization
  // read configure
  if (argc > 1)
    config(argv[1]);
  // prepare shared memory
  // daemonlize, check and fork
  while(1) {

    sleep(5);
  }
}
