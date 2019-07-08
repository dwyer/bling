#pragma once

#define $import(...)
#define $package(_)

#include "runtime/runtime.h"

extern int len(slice_t s);
extern int cap(slice_t s);
extern void *get_ptr(slice_t s, int index);
extern slice_t append(slice_t s, void *obj);
extern void print(char *fmt, ...);
extern void panic(char *fmt, ...);

extern slice_t make_slice(int size, int len, int cap);
extern map_t make_map(const desc_t *key_desc, const desc_t *val_desc);
