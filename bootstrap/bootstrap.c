#include "bootstrap.h"

#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>

extern void print(const char *s) {
    fprintf(stderr, "%s\n", s);
}

extern void panic(const char *s) {
    print(s);
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
