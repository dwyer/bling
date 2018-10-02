#pragma once
#include "builtin/builtin.h"

typedef struct {
    char *src;
    int rd_offset;
    int offset;
    int ch;
    char lit[BUFSIZ];
} scanner_t;

int scanner_scan(scanner_t *s, char **lit);
void scanner_init(scanner_t *s, char *src);
