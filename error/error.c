#include "error/error.h"

extern int errno; // from libc
extern char *strerror(int); // from libc

extern error_t *error_make(const char *error) {
    error_t err = {
        .error = strdup(error),
    };
    return esc(err);
}

extern void error_move(error_t *src, error_t **dst) {
    if (dst != NULL) {
        *dst = src;
    } else {
        panic("Unhandled error: %s", src->error);
    }
}

extern void error_check(error_t **e) {
    if (errno) {
        error_t *err = error_make(strerror(errno));
        error_move(err, e);
    }
}

extern void error_clear() {
    errno = 0;
}

extern void error_free(error_t *e) {
    free(e->error);
    free(e);
}
