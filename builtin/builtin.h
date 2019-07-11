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
extern slice_t append(slice_t s, void *obj);
extern void print(char *fmt, ...);
extern void panic(char *fmt, ...);
extern bool streq(const char *a, const char *b);

extern slice_t make_slice(int size, int len, int cap);
extern map_t make_map(const desc_t *key_desc, const desc_t *val_desc);

extern error_t *make_error(const char *error);
extern void free_error(error_t *error);

extern const char *filepath_ext(const char *filename); // TODO move this
extern bool is_ext(const char *filename, const char *ext); // TODO move this
