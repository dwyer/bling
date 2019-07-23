#pragma once

#include "bootstrap/bootstrap.h"

typedef struct {
    char *error;
} error_t;

extern error_t *error_make(const char *error);
extern void error_move(error_t *src, error_t **dst);
extern void error_free(error_t *e);

// clearing and checking errno
extern void error_clear();
extern void error_check(error_t **e);
