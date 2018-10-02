#pragma once

#include "kc.h"

extern slice_t types;
extern char *src;
extern int rd_offset;
extern int offset;
extern int ch;
extern char lit[BUFSIZ];
extern int tok;
extern bool is_type;

int line(void);
int col(void);
void next(void);
void scan(void);
