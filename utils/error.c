#include "utils/utils.h"

extern int errno; // from libc
extern char *strerror(int); // from libc

extern utils$Error *utils$NewError(const char *msg) {
    utils$Error err = {
        .error = strdup(msg),
    };
    return esc(err);
}

extern void utils$Error_move(utils$Error *src, utils$Error **dst) {
    if (dst != NULL) {
        *dst = src;
    } else {
        panic("Unhandled error: %s", src->error);
    }
}

extern void utils$Error_check(utils$Error **e) {
    if (errno) {
        utils$Error *err = utils$NewError(strerror(errno));
        utils$Error_move(err, e);
    }
}

extern void utils$clearError() {
    errno = 0;
}

extern void utils$Error_free(utils$Error *e) {
    free(e->error);
    free(e);
}
