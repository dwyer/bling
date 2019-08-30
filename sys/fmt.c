#include "gen/sys/sys.h"

#include <stdarg.h>
#include <stdio.h>

extern void sys$printf(const char *s, ...) {
    va_list ap;
    va_start(ap, s);
    vprintf(s, ap);
    va_end(ap);
}

extern char *sys$sprintf(const char *s, ...) {
    char *res = NULL;
    va_list ap;
    va_start(ap, s);
    vasprintf(&res, s, ap);
    va_end(ap);
    return res;
}
