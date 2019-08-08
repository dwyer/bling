#pragma once
#include "bootstrap/bootstrap.h"

package(utils);

typedef struct {
    int size;
    int len;
    int cap;
    void *array;
} utils$Slice_Slice;

extern utils$Slice_Slice utils$Slice_init(int size);
extern void utils$Slice_deinit(utils$Slice_Slice *s);

extern int utils$Slice_len(const utils$Slice_Slice *s);
extern int utils$Slice_cap(const utils$Slice_Slice *s);
extern void *utils$Slice_ref(const utils$Slice_Slice *s, int i);
extern void utils$Slice_get(const utils$Slice_Slice *s, int i, void *dst);

extern void utils$Slice_set_len(utils$Slice_Slice *s, int len);
extern void utils$Slice_set(utils$Slice_Slice *s, int i, const void *x);
extern void utils$Slice_append(utils$Slice_Slice *s, const void *x);
extern void *utils$Slice_to_nil_array(utils$Slice_Slice s);
