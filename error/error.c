#include "error/error.h"
#include "builtin/builtin.h"

extern error_t *make_error(const char *error) {
    error_t err = {
        .error = strdup(error),
    };
    return esc(err);
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
