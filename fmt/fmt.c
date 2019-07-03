#include "fmt/fmt.h"

extern char *fmt_sprintf(const char *fmt, ...) {
    char *ret = NULL;
    va_list ap;
    va_start(ap, fmt);
    vasprintf(&ret, fmt, ap);
    va_end(ap);
    return ret;
}
