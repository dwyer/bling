#pragma once
#include "bling/token/token.h"

package(scanner);

import("bling/token");
import("sys");

typedef struct {
    token$File *file;
    char *src;
    int rd_offset;
    int offset;
    int ch;
    bool insertSemi;
    bool dontInsertSemis;
} scanner$Scanner;

extern token$Token scanner$scan(scanner$Scanner *s, token$Pos *pos, char **lit);
extern void scanner$init(scanner$Scanner *s, token$File *file, char *src);
