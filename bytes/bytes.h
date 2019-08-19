#pragma once
#include "utils/utils.h"

package(bytes);

import("sys");
import("utils");

extern bool bytes$hasSuffix(const char *b, const char *suffix);
extern int bytes$indexByte(const char *b, char c);
extern char *bytes$join(const char *a[], int size, const char *sep);
extern int bytes$lastIndexByte(const char *b, char c);

typedef utils$Slice bytes$Buffer;

extern char *bytes$Buffer_bytes(bytes$Buffer *b);
extern int bytes$Buffer_len(bytes$Buffer *b);
extern char *bytes$Buffer_string(bytes$Buffer *b);
extern int bytes$Buffer_write(bytes$Buffer *b, const char *p, int size, utils$Error **error);
extern void bytes$Buffer_writeByte(bytes$Buffer *b, char p, utils$Error **error);
