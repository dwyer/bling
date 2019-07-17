#include "builtin/builtin.h"

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

extern error_t *make_error(const char *error) {
    error_t err = {
        .error = strdup(error),
    };
    return memdup(&err, sizeof(error_t));
}

extern error_t *make_sysError(void) {
    return make_error(strerror(errno));
}

extern void error_move(error_t *src, error_t **dst) {
    if (dst != NULL) {
        *dst = src;
    }
}

extern void error_free(error_t *error) {
    free(error->error);
    free(error);
}

extern bool streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}
