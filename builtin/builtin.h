#pragma once

#define $import(...)
#define $package(_)

#include "runtime/runtime.h"

$import("runtime");

typedef struct {
    char *error;
} error_t;

extern int len(slice_t s);
extern int cap(slice_t s);
extern void *get_ptr(slice_t s, int index);
extern slice_t append(slice_t s, const void *obj);
extern void print(const char *fmt, ...);
extern void panic(const char *fmt, ...);
extern bool streq(const char *a, const char *b);

extern slice_t make_slice(int size, int len, int cap);

extern error_t *make_error(const char *error);
extern error_t *make_sysError(void);
extern void error_move(error_t *src, error_t **dst);
extern void error_free(error_t *error);
