#pragma once

#include "bytes/bytes.h"
#include "utils/utils.h"

package(strings);

import("bytes");
import("errors");

extern char *strings$join(const char *a[], int size, const char *sep);

typedef struct {
    utils$Slice _buf;
} strings$Builder;

extern int strings$Builder_len(strings$Builder *b);
extern char *strings$Builder_string(strings$Builder *b);
extern int strings$Builder_write(strings$Builder *b, const char *p, int size, errors$Error **error);
extern void strings$Builder_writeByte(strings$Builder *b, char p, errors$Error **error);
