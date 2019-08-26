#include "utils/utils.h"

#include "sys/sys.h"

extern utils$Error *utils$NewError(const char *msg) {
    utils$Error err = {
        .error = sys$strdup(msg),
    };
    return esc(err);
}

extern void utils$Error_move(utils$Error *src, utils$Error **dst) {
    if (dst != NULL) {
        *dst = src;
    } else {
        panic(sys$sprintf("Unhandled error: %s", src->error));
    }
}

extern void utils$Error_check(utils$Error **e) {
    if (sys$errno()) {
        utils$Error *err = utils$NewError(sys$errnoString());
        utils$Error_move(err, e);
    }
}

extern void utils$clearError() {
    sys$errnoReset();
}

extern void utils$Error_free(utils$Error *e) {
    free(e->error);
    free(e);
}
