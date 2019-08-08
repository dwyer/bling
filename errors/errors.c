#include "errors/errors.h"

extern int errno; // from libc
extern char *strerror(int); // from libc

extern errors$Error *errors$NewError(const char *msg) {
    errors$Error err = {
        .error = strdup(msg),
    };
    return esc(err);
}

extern void errors$Error_move(errors$Error *src, errors$Error **dst) {
    if (dst != NULL) {
        *dst = src;
    } else {
        panic("Unhandled error: %s", src->error);
    }
}

extern void errors$Error_check(errors$Error **e) {
    if (errno) {
        errors$Error *err = errors$NewError(strerror(errno));
        errors$Error_move(err, e);
    }
}

extern void errors$clearError() {
    errno = 0;
}

extern void errors$Error_free(errors$Error *e) {
    free(e->error);
    free(e);
}
