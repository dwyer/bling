#include "error/error.h"

extern int errno; // from libc
extern char *strerror(int); // from libc

extern error$Error *error$make(const char *error) {
    error$Error err = {
        .error = strdup(error),
    };
    return esc(err);
}

extern void error$move(error$Error *src, error$Error **dst) {
    if (dst != NULL) {
        *dst = src;
    } else {
        panic("Unhandled error: %s", src->error);
    }
}

extern void error$check(error$Error **e) {
    if (errno) {
        error$Error *err = error$make(strerror(errno));
        error$move(err, e);
    }
}

extern void error$clear() {
    errno = 0;
}

extern void error$free(error$Error *e) {
    free(e->error);
    free(e);
}
