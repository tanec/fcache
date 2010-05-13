#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "Config.h"
#include "fastcgi-api.h"
#include "event-api.h"
#include "log.h"
#include "util.h"
#include "fcache.h"

setting_t settings;

int
main(int argc, char**argv)
{
  int c;

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

  if (settings.daemon)
    daemonize(1,1);
  // files for log
  
  
  // Initialization
  // prepare shared memory
  // daemonlize, check and fork
  while(1) {
    sleep(5);
  }
  
  return EXIT_SUCCESS;
}
