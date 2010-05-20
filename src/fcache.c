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

#include "fcache.h"
#include "settings.h"
#include "process.h"

stat_summary_t stat_mem, stat_fs, stat_http, stat_auth;

int
main(int argc, char**argv)
{
  int c;
  struct rlimit rlim;

  /* process arguments */
  while (-1 != (c = getopt(argc, argv,
                           "C:"  /* config file */
                           "d"  /* goto daemon mode*/
                           "I:" /*interface to bind*/
                           "p:" /* TCP port number to listen on */
			   "s:" /* unix socket path to listen on */

			   "c:"  /* max simultaneous connections */
                           "P:"  /* file to hold pid */
                           "t:"  /* max thread connections */
			   ))) {
    switch (c) {
    case 'C':
        read_cfg(&cfg, optarg);
    case 'd':
      cfg.daemon = 1;

    case 'I':
      cfg.bind_addr = optarg;
      break;
    case 'p':
      cfg.port = atoi(optarg);
      cfg.conn_type = tcp;
      break;
    case 's':
      cfg.socketpath = optarg;
      cfg.conn_type = domain_socket;
      break;

    case 'c':
      cfg.maxconns = atoi(optarg);
      break;

    case 'P':
      cfg.pid_file = optarg;
      break;

    case 't':
      cfg.num_threads = atoi(optarg);
      if (cfg.num_threads <= 0) {
	fprintf(stderr, "Number of threads must be greater than 0\n");
	return 1;
      }
      if (cfg.num_threads > 64) {
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
      int maxfiles = cfg.maxconns;
      if (rlim.rlim_cur < maxfiles)
          rlim.rlim_cur = maxfiles;
      if (rlim.rlim_max < rlim.rlim_cur)
          rlim.rlim_max = rlim.rlim_cur;
      if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
          fprintf(stderr, "failed to set rlimit for open files. Try running as root or requesting smaller maxconns value.\n");
          exit(EX_OSERR);
      }
  }

  if (cfg.daemon)
    daemonize(1,1);
  // files for log
  
  
  // Initialization
{
request_t req = {"gbkkk", "--hosttt", "http://host:port/path/file"};
printf("time=%d\n", time(NULL));
process_init();
process(&req, NULL);
}
  // prepare shared memory
  // daemonlize, check and fork
  while(cfg.daemon) {
    sleep(5);
  }
  
  return EXIT_SUCCESS;
}
