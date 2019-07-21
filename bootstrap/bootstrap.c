#include "bootstrap.h"

extern int backtrace(void **, int); // libc
extern void backtrace_symbols_fd(void* const*, int, int); // libc
extern void exit(int) /* __attribute__((noreturn)) */; // lib

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
    int fd = 2; //stderr
    backtrace_symbols_fd(buf, n, fd);
    exit(1);
}

extern int strcmp(const char *, const char *);
extern bool streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}
