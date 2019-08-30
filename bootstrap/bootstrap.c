#include "bootstrap.h"

#include <execinfo.h> // backtrace, etc.
#include <stdio.h> // fprintf, BUFSIZ
#include <stdlib.h> // exit
#include <unistd.h> // STDERR_FILENO

typedef char *charptr;
typedef void *voidptr;

extern void print(const char *s) {
    fprintf(stderr, "%s\n", s);
}

extern void printbacktrace() {
    void *buf[BUFSIZ];
    int n = backtrace(buf, BUFSIZ);
    backtrace_symbols_fd(buf, n, STDERR_FILENO);
}

extern void panic(const char *s) {
    print(s);
    printbacktrace();
    exit(1);
}
