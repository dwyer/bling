#pragma once

#include "builtin/builtin.h"

$import("builtin");

typedef struct {
    int size;
    int len;
    int cap;
    void *array;
} slice_t;

extern slice_t slice_init(int size, int len, int cap);
extern void slice_deinit(slice_t *s);

extern int slice_len(const slice_t *s);
extern int slice_cap(const slice_t *s);
extern void *slice_ref(const slice_t *s, int i);
extern void slice_get(const slice_t *s, int i, void *dst);

extern void slice_set_len(slice_t *s, int len);
extern void slice_set(slice_t *s, int i, const void *x);
extern void slice_append(slice_t *s, const void *x);
extern void *slice_to_nil_array(slice_t s);

extern int len(slice_t s);
extern int cap(slice_t s);
extern void *get_ptr(slice_t s, int index);
extern slice_t append(slice_t s, const void *obj);

extern slice_t make_slice(int size, int len, int cap);
