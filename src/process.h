#ifndef PROCESS_H
#define PROCESS_H

typedef struct {
    char *enc;
    char *host;
    char *uri;
    char *keyword;
} request_t;

typedef struct {

} response_t;

void process(request_t *, response_t *);

#endif // PROCESS_H
