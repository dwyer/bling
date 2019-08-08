#pragma once

#include "error/error.h"
#include "slice/slice.h"

package(bytes);

import("error");
import("slice");

extern bool bytes$hasSuffix(const char *s, const char *suffix);
extern char *bytes$join(const char *a[], int size, const char *sep);

typedef slice$Slice bytes$Buffer;

extern char *bytes$Buffer_bytes(bytes$Buffer *b);
extern int bytes$Buffer_len(bytes$Buffer *b);
extern char *bytes$Buffer_string(bytes$Buffer *b);
extern int bytes$Buffer_write(bytes$Buffer *b, const char *p, int size, error$Error **error);
extern void bytes$Buffer_writeByte(bytes$Buffer *b, char p, error$Error **error);
