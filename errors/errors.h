#pragma once

#include "bootstrap/bootstrap.h"

package(error);

typedef struct {
    char *error;
} errors$Error;

extern errors$Error *errors$make(const char *error);
extern void errors$move(errors$Error *src, errors$Error **dst);
extern void errors$free(errors$Error *e);

// clearing and checking errno
extern void errors$clear();
extern void errors$check(errors$Error **e);
