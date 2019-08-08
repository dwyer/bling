#include "slice/slice.h"

extern slice$Slice slice$init(int size) {
    slice$Slice s = {
        .size = size,
        .len = 0,
        .cap = 0,
        .array = NULL,
    };
    return s;
}

extern void slice$deinit(slice$Slice *s) {
    free(s->array);
}

extern int slice$len(const slice$Slice *s) {
    return s->len;
}

extern int slice$cap(const slice$Slice *s) {
    return s->cap;
}

extern void *slice$ref(const slice$Slice *s, int i) {
    return (void *)&((char *)s->array)[i * s->size];
}

extern void slice$get(const slice$Slice *s, int i, void *dst) {
    if (i >= s->len) {
        panic("out of range: index=%d, len=%d", i, s->len);
    }
    if (s->size == 1) {
         *(char *)dst = ((char *)s->array)[i];
    } else {
        memcpy(dst, slice$ref(s, i), s->size);
    }
}

static void slice$set_cap(slice$Slice *s, int cap) {
    s->cap = cap;
    s->array = realloc(s->array, s->cap * s->size);
}

static void set_len(slice$Slice *s, int len) {
    bool grow = false;
    int cap = s->cap;
    if (cap == 0) {
        cap = 1;
        grow = true;
    }
    while (cap < len) {
        cap *= 2;
        grow = true;
    }
    if (s->array == NULL || grow) {
        slice$set_cap(s, cap);
    }
    s->len = len;
}

extern void slice$set_len(slice$Slice *s, int len) {
    int old = s->len;
    set_len(s, len);
    int diff = len - old;
    if (diff > 0) {
        memset(slice$ref(s, old), 0, diff * s->size);
    }
}

extern void slice$set(slice$Slice *s, int i, const void *x) {
    if (s->size == 1) {
        ((char *)s->array)[i] = *(char *)x;
    } else {
        memcpy(slice$ref(s, i), x, s->size);
    }
}

extern void slice$append(slice$Slice *s, const void *x) {
    set_len(s, s->len + 1);
    slice$set(s, s->len - 1, x);
}

extern void *slice$to_nil_array(slice$Slice s) {
    void *nil = NULL;
    slice$append(&s, &nil);
    return s.array;
}
