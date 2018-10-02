#pragma once
#include "builtin/builtin.h"

typedef struct {
    char *src;
    int rd_offset;
    int offset;
    int ch;
    char lit[BUFSIZ];
    int tok;
} scanner_t;

extern char *src;
extern int rd_offset;
extern int offset;
extern int ch;
extern char lit[BUFSIZ];
extern int tok;

void next(void);
void scan(void);
