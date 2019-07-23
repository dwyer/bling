#pragma once

#include "error/error.h"
#include "slice/slice.h"

import("error");
import("slice");

extern char *bytes_join(const char *a[], int size, const char *sep);

typedef slice_t buffer_t;

extern char *buffer_bytes(buffer_t *b);
extern int buffer_len(buffer_t *b);
extern char *buffer_string(buffer_t *b);
extern int buffer_write(buffer_t *b, const char *p, int size, error_t **error);
extern void buffer_writeByte(buffer_t *b, char p, error_t **error);
