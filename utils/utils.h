#pragma once
#include "bootstrap/bootstrap.h"

package(utils);

typedef struct {
    int size;
    int len;
    int cap;
    void *array;
} utils$Slice;

extern utils$Slice utils$init(int size);
extern void utils$deinit(utils$Slice *s);

extern int utils$len(const utils$Slice *s);
extern int utils$cap(const utils$Slice *s);
extern void *utils$ref(const utils$Slice *s, int i);
extern void utils$get(const utils$Slice *s, int i, void *dst);

extern void utils$set_len(utils$Slice *s, int len);
extern void utils$set(utils$Slice *s, int i, const void *x);
extern void utils$append(utils$Slice *s, const void *x);
extern void *utils$to_nil_array(utils$Slice s);
