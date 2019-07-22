#include "fmt/fmt.h"

#include <stdarg.h>
#include <stdio.h>

extern void fmt_printf(const char *s, ...) {
    va_list ap;
    va_start(ap, s);
    vprintf(s, ap);
    va_end(ap);
}

extern char *fmt_sprintf(const char *s, ...) {
    char *res = NULL;
    va_list ap;
    va_start(ap, s);
    vasprintf(&res, s, ap);
    va_end(ap);
    return res;
}
