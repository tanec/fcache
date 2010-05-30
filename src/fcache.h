#ifndef FCACHE_H
#define FCACHE_H

#include <sys/types.h>

#define EX_OSERR 3

enum conn_states {
  conn_listening,  /**< the socket which listens for connections */
  conn_new_cmd,    /**< Prepare connection for next command */
  conn_waiting,    /**< waiting for a readable socket */
  conn_read,       /**< reading in a command line */
  conn_parse_cmd,  /**< try to parse a command from the input buffer */
  conn_write,      /**< writing out a simple response */
  conn_nread,      /**< reading in a fixed number of bytes */
  conn_swallow,    /**< swallowing unnecessary bytes w/o storing */
  conn_closing,    /**< closing this connection */
  conn_mwrite,     /**< writing out many items sequentially */
  conn_max_state   /**< Max state value (used for assertion) */
};

enum bin_substates {
  bin_no_state,
  bin_reading_set_header,
  bin_reading_cas_header,
  bin_read_set_value,
  bin_reading_get_key,
  bin_reading_stat,
  bin_reading_del_header,
  bin_reading_incr_header,
  bin_read_flush_exptime,
  bin_reading_sasl_auth,
  bin_reading_sasl_auth_data
};

typedef struct conn conn;
struct conn {
  int    sfd;
  enum conn_states  state;
  enum bin_substates substate;
  struct event event;
  short  ev_flags;
  short  which;   /** which events were just triggered */

  char   *rbuf;   /** buffer to read commands into */
  char   *rcurr;  /** but if we parsed some already, this is where we stopped */
  int    rsize;   /** total allocated size of rbuf */
  int    rbytes;  /** how much data, starting from rcur, do we have unparsed */

  char   *wbuf;
  char   *wcurr;
  int    wsize;
  int    wbytes;
  /** which state to go into after finishing current write */
  enum conn_states  write_and_go;
  void   *write_and_free; /** free this memory after finishing writing */

  char   *ritem;  /** when we read in an item's value, it goes here */
  int    rlbytes;

  /* data for the nread state */

  /**
     * item is used to hold an item structure created after reading the command
     * line of set/add/replace commands, but before we finished reading the actual
     * data. The data is read into ITEM_data(item) to avoid extra copying.
     */

  void   *item;     /* for commands set/add/replace  */

  /* data for the swallow state */
  int    sbytes;    /* how many bytes to swallow */

  /* data for the mwrite state */
  struct iovec *iov;
  int    iovsize;   /* number of elements allocated in iov[] */
  int    iovused;   /* number of elements used in iov[] */

  struct msghdr *msglist;
  int    msgsize;   /* number of elements allocated in msglist[] */
  int    msgused;   /* number of elements used in msglist[] */
  int    msgcurr;   /* element in msglist[] being transmitted now */
  int    msgbytes;  /* number of bytes in current msg */

  int    isize;
  int    ileft;

  char   **suffixlist;
  int    suffixsize;
  char   **suffixcurr;
  int    suffixleft;


  /* data for UDP clients */
  int    request_id; /* Incoming UDP request ID, if this is a UDP "connection" */
  unsigned char *hdrbuf; /* udp packet headers */
  int    hdrsize;   /* number of headers' worth of space is allocated */

  bool   noreply;   /* True if the reply should not be sent. */
  /* current stats command */
  struct {
    char *buffer;
    size_t size;
    size_t offset;
  } stats;

  /* Binary protocol stuff */
  /* This is where the binary header goes */
  short cmd; /* current command being processed */
  int opaque;
  int keylen;
  conn   *next;     /* Used for generating a list of conn structures */
};

#endif // FCACHE_H
