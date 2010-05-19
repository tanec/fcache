#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "Config.h"
#include "fastcgi-api.h"
#include "event-api.h"
#include "log.h"
#include "util.h"
#include "md5.h"


#include "fcache.h"
#include "process.h"

setting_t settings;
stat_summary_t stat_mem, stat_fs, stat_http, stat_auth;


int
main(int argc, char**argv)
{
  int c;
  struct rlimit rlim;

  //defaults
  settings.daemon = 1;
  settings.conn_type = tcp;
  settings.bind_addr = "127.0.0.1";
  settings.port = 2012;
  settings.socketpath = "/tmp/fcache.socket";
  settings.pid_file = "/var/run/fcache.pid";
  settings.cache_file = "fcache.store";
  settings.maxconns = 4096;

  /* process arguments */
  while (-1 != (c = getopt(argc, argv,
			   "D"  /*do not goto daemon mode*/
                           "I:" /*interface to bind*/
                           "p:" /* TCP port number to listen on */
			   "s:" /* unix socket path to listen on */

			   "c:"  /* max simultaneous connections */
                           "P:"  /* file to hold pid */
                           "f:"  /* file to store cache data */
                           "t:"  /* max thread connections */
			   ))) {
    switch (c) {
    case 'D':
      settings.daemon = 0;

    case 'I':
      settings.bind_addr = optarg;
      break;
    case 'p':
      settings.port = atoi(optarg);
      settings.conn_type = tcp;
      break;
    case 's':
      settings.socketpath = optarg;
      settings.conn_type = domain_socket;
      break;

    case 'c':
      settings.maxconns = atoi(optarg);
      break;

    case 'P':
      settings.pid_file = optarg;
      break;
    case 'f':
      settings.cache_file = optarg;
      break;

    case 't':
      settings.num_threads = atoi(optarg);
      if (settings.num_threads <= 0) {
	fprintf(stderr, "Number of threads must be greater than 0\n");
	return 1;
      }
      if (settings.num_threads > 64) {
	fprintf(stderr, "WARNING: Setting a high number of worker"
		"threads is not recommended.\n"
		" Set this value to the number of cores in"
		" your machine or less.\n");
      }
      break;
    default: return 1;
    }
  }

  /*
   * If needed, increase rlimits to allow as many connections
   * as needed.
   */
  if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
      fprintf(stderr, "failed to getrlimit number of files\n");
      exit(EX_OSERR);
  } else {
      int maxfiles = settings.maxconns;
      if (rlim.rlim_cur < maxfiles)
          rlim.rlim_cur = maxfiles;
      if (rlim.rlim_max < rlim.rlim_cur)
          rlim.rlim_max = rlim.rlim_cur;
      if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
          fprintf(stderr, "failed to set rlimit for open files. Try running as root or requesting smaller maxconns value.\n");
          exit(EX_OSERR);
      }
  }


  if (settings.daemon)
    daemonize(1,1);
  // files for log
  
  
  // Initialization

process(NULL, NULL);
  // prepare shared memory
  // daemonlize, check and fork
  while(1) {
    sleep(5);
  }
  
  return EXIT_SUCCESS;
}
