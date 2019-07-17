#include "builtin/builtin.h"

extern void *memdup(const void *src, size_t size) {
    return memcpy(malloc(size), src, size);
}

static void vprint(const char *fmt, va_list ap) {
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

extern void print(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
}

extern void panic(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
    void *buf[1024];
    int n = backtrace(buf, 1024);
    backtrace_symbols_fd(buf, n, 2);
    exit(1);
}

extern bool streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}
