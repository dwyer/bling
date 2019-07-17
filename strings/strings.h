#pragma once

#include "slice/slice.h"

extern char *strings_join(const char *a[], int size, const char *sep);

typedef struct {
    slice_t _buf;
} strings_Builder;

extern int strings_Builder_len(strings_Builder *b);
extern char *strings_Builder_string(strings_Builder *b);
extern int strings_Builder_write(strings_Builder *b, const char *p, int size, error_t **error);
extern void strings_Builder_writeByte(strings_Builder *b, char p, error_t **error);
