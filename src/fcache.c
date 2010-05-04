#include "Config.h"
#include <stdio.h>
#include "fastcgi-api.h"
#include "event-api.h"
#include "curl-api.h"

int main() {
    serve();
    return 0;
}
