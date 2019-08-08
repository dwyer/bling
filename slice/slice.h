#pragma once
#include "bootstrap/bootstrap.h"

package(slice);

typedef struct {
    int size;
    int len;
    int cap;
    void *array;
} slice$Slice;

extern slice$Slice slice$init(int size);
extern void slice$deinit(slice$Slice *s);

extern int slice$len(const slice$Slice *s);
extern int slice$cap(const slice$Slice *s);
extern void *slice$ref(const slice$Slice *s, int i);
extern void slice$get(const slice$Slice *s, int i, void *dst);

extern void slice$set_len(slice$Slice *s, int len);
extern void slice$set(slice$Slice *s, int i, const void *x);
extern void slice$append(slice$Slice *s, const void *x);
extern void *slice$to_nil_array(slice$Slice s);
