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

typedef struct {
    pos_t pos;
    token_t tok;
    char *lit;
} scanner_result_t;

extern token_t scanner_scan(scanner_t *s, pos_t *pos, char **lit);
extern void scanner_init(scanner_t *s, char *src);
