#pragma once
#include "bootstrap/bootstrap.h"

package(utils);

typedef struct {
    int size;
    int len;
    int cap;
    void *array;
} utils$Slice;

extern utils$Slice utils$Slice_init(int size);
extern void utils$Slice_deinit(utils$Slice *s);

extern int utils$Slice_len(const utils$Slice *s);
extern int utils$Slice_cap(const utils$Slice *s);
extern void *utils$Slice_ref(const utils$Slice *s, int i);
extern void utils$Slice_get(const utils$Slice *s, int i, void *dst);

extern void utils$Slice_set_len(utils$Slice *s, int len);
extern void utils$Slice_set(utils$Slice *s, int i, const void *x);
extern void utils$Slice_append(utils$Slice *s, const void *x);
extern void *utils$Slice_to_nil_array(utils$Slice s);
