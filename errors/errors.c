#include "errors/errors.h"

extern int errno; // from libc
extern char *strerror(int); // from libc

extern errors$Error *errors$make(const char *msg) {
    errors$Error err = {
        .error = strdup(msg),
    };
    return esc(err);
}

extern void errors$move(errors$Error *src, errors$Error **dst) {
    if (dst != NULL) {
        *dst = src;
    } else {
        panic("Unhandled error: %s", src->error);
    }
}

extern void errors$check(errors$Error **e) {
    if (errno) {
        errors$Error *err = errors$make(strerror(errno));
        errors$move(err, e);
    }
}

extern void errors$clear() {
    errno = 0;
}

extern void errors$free(errors$Error *e) {
    free(e->error);
    free(e);
}
