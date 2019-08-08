#pragma once

#include "bootstrap/bootstrap.h"

package(error);

typedef struct {
    char *error;
} error$Error;

extern error$Error *error$make(const char *error);
extern void error$move(error$Error *src, error$Error **dst);
extern void error$free(error$Error *e);

// clearing and checking errno
extern void error$clear();
extern void error$check(error$Error **e);
