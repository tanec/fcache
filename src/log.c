#include <sys/types.h>
#include <stdarg.h>
#include <unistd.h>
#include "log.h"

void log_stderr(err_t err, const char *fmt, ...)
{
    u_char   *p, *last;
    va_list   args;
    u_char    errstr[MAX_ERROR_NUM];

    last = errstr + MAX_ERROR_NUM;

    va_start(args, fmt);
    p = ngx_vslprintf(errstr, last, fmt, args);
    va_end(args);

    if (err) {
        p = ngx_log_errno(p, last, err);
    }

    if (p > last - LINEFEED_SIZE) {
        p = last - LINEFEED_SIZE;
    }

    ngx_linefeed(p);

    (void) ngx_write_console(STDERR_FILENO, errstr, p - errstr);
}
