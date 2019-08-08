#pragma once
#include "bling/token/token.h"

import("bling/token");

typedef struct {
    token$File *file;
    char *src;
    int rd_offset;
    int offset;
    int ch;
    bool insertSemi;
    bool dontInsertSemis;
} scanner_t;

extern token$Token scanner_scan(scanner_t *s, token$Pos *pos, char **lit);
extern void scanner_init(scanner_t *s, token$File *file, char *src);
