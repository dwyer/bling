#include "bootstrap.h"

#include <execinfo.h> // backtrace, etc.
#include <stdarg.h>
#include <stdio.h> // fprintf, BUFSIZ
#include <stdlib.h> // exit
#include <unistd.h> // STDERR_FILENO

extern void print(const char *s) {
    fprintf(stderr, "%s\n", s);
}

extern void panic(const char *s) {
    print(s);
    void *buf[BUFSIZ];
    int n = backtrace(buf, BUFSIZ);
    backtrace_symbols_fd(buf, n, STDERR_FILENO);
    exit(1);
}

extern int strcmp(const char *, const char *);
extern bool streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}
