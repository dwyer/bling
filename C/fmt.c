#include "gen/C/C.h"

#include <stdarg.h>
#include <stdio.h>

extern void C$printf(const char *s, ...) {
    va_list ap;
    va_start(ap, s);
    vprintf(s, ap);
    va_end(ap);
}

extern char *C$sprintf(const char *s, ...) {
    char *res = nil;
    va_list ap;
    va_start(ap, s);
    vasprintf(&res, s, ap);
    va_end(ap);
    return res;
}
