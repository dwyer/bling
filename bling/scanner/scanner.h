#pragma once
#include "bling/token/token.h"

import("bling/token");

typedef struct {
    char *src;
    int rd_offset;
    int offset;
    int ch;
    bool insertSemi;
    bool dontInsertSemis;
} scanner_t;

extern token_t scanner_scan(scanner_t *s, char **lit);
extern void scanner_init(scanner_t *s, char *src);
