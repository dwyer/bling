#pragma once

#include "bootstrap/bootstrap.h"

package(error);

typedef struct {
    char *error;
} errors$Error;

extern errors$Error *errors$NewError(const char *error);
extern void errors$Error_move(errors$Error *src, errors$Error **dst);
extern void errors$Error_free(errors$Error *e);

// clearing and checking errno
extern void errors$clearError();
extern void errors$Error_check(errors$Error **e);
