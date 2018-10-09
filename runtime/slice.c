#include "runtime.h"

#include <stdlib.h>
#include <string.h>

extern slice_t slice_init(const desc_t *desc, int len, int cap) {
    slice_t s = {
        .len = 0,
        .cap = 0,
        .array = NULL,
        .desc = desc,
    };
    slice_set_len(&s, len);
    return s;
}

extern void slice_deinit(slice_t *s) {
    free(s->array);
}

extern int slice_len(const slice_t *s) {
    return s->len;
}

extern int slice_cap(const slice_t *s) {
    return s->cap;
}

extern void *slice_ref(const slice_t *s, int i) {
    return (void *)&((char *)s->array)[i * s->desc->size];
}

extern void slice_get(const slice_t *s, int i, void *dst) {
    memcpy(dst, slice_ref(s, i), s->desc->size);
}

static void slice_set_cap(slice_t *s, int cap) {
    s->cap = cap;
    s->array = realloc(s->array, s->cap * s->desc->size);
}

static void set_len(slice_t *s, int len) {
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
        slice_set_cap(s, cap);
    }
    s->len = len;
}

extern void slice_set_len(slice_t *s, int len) {
    int old = s->len;
    set_len(s, len);
    int diff = len - old;
    if (diff > 0) {
        memset(slice_ref(s, old), 0, diff * s->desc->size);
    }
}

extern void slice_set(slice_t *s, int i, const void *x) {
    memcpy(slice_ref(s, i), x, s->desc->size);
}

extern void slice_append(slice_t *s, const void *x) {
    set_len(s, s->len + 1);
    slice_set(s, s->len - 1, x);
}
