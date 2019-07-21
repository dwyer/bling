#pragma once

#include "bootstrap/bootstrap.h"

$import("builtin");

typedef struct {
    char *error;
} error_t;

extern error_t *make_error(const char *error);
extern error_t *make_sysError();
extern void error_move(error_t *src, error_t **dst);
extern void error_free(error_t *error);
