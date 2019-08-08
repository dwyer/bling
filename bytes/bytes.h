#pragma once

#include "errors/errors.h"
#include "utils/utils.h"

package(bytes);

import("errors");
import("utils");

extern bool bytes$hasSuffix(const char *s, const char *suffix);
extern char *bytes$join(const char *a[], int size, const char *sep);

typedef utils$Slice_Slice bytes$Buffer;

extern char *bytes$Buffer_bytes(bytes$Buffer *b);
extern int bytes$Buffer_len(bytes$Buffer *b);
extern char *bytes$Buffer_string(bytes$Buffer *b);
extern int bytes$Buffer_write(bytes$Buffer *b, const char *p, int size, errors$Error **error);
extern void bytes$Buffer_writeByte(bytes$Buffer *b, char p, errors$Error **error);
