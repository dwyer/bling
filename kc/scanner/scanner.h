#pragma once
#include "builtin/builtin.h"
#include "kc/token/token.h"

typedef struct {
    char *src;
    int rd_offset;
    int offset;
    int ch;
} scanner_t;

extern token_t scanner_scan(scanner_t *s, char **lit);
extern void scanner_init(scanner_t *s, char *src);
