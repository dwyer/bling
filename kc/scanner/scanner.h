#pragma once
#include "builtin/builtin.h"

typedef struct {
    char *src;
    int rd_offset;
    int offset;
    int ch;
    char lit[BUFSIZ];
} scanner_t;

void next(scanner_t *s);
int scan(scanner_t *s, char **lit);
