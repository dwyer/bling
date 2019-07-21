#include "error/error.h"

extern int errno; // from libc
extern char *strerror(int); // from libc

extern error_t *make_error(const char *error) {
    error_t err = {
        .error = strdup(error),
    };
    return esc(err);
}

extern error_t *make_sysError() {
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
