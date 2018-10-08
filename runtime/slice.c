#include "runtime.h"

#include <stdlib.h>
#include <string.h>

extern slice_t slice_init(const desc_t *desc) {
    slice_t s = {
        .len = 0,
        .cap = 0,
        .array = NULL,
        .desc = desc,
    };
    return s;
}

extern void slice_deinit(slice_t *s) {
    if (s->desc->deinit) {
        for (int i = 0; i < slice_len(s); i++) {
            s->desc->deinit(slice_ref(s, i));
        }
    }
    free(s->array);
}

extern int slice_len(const slice_t *s) {
    return s->len;
}

extern int slice_cap(const slice_t *s) {
    return s->cap;
}

extern void *slice_ref(const slice_t *s, int i) {
    return &s->array[i * s->desc->size];
}

extern void slice_get(const slice_t *s, int i, void *dst) {
    memcpy(dst, slice_ref(s, i), s->desc->size);
}

extern void set_cap(slice_t *s, int cap) {
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
        set_cap(s, cap);
    }
    s->len = len;
}

extern void slice_set_len(slice_t *s, int len) {
    int old = s->len;
    set_len(s, len);
    int diff = len - old;
    if (diff > 0) {
        if (s->desc->init) {
            for (int i = old; i < s->len; i++) {
                s->desc->init(slice_ref(s, i));
            }
        } else {
            memset(slice_ref(s, old), 0, diff * s->desc->size);
        }
    }
}

extern void slice_set(slice_t *s, int i, const void *x) {
    desc_cpy(s->desc, slice_ref(s, i), x);
}

extern void slice_append(slice_t *s, const void *x) {
    slice_set_len(s, s->len + 1);
    slice_set(s, s->len - 1, x);
}

extern void slice_pop(slice_t *s, void *dst) {
    desc_cpy(s->desc, dst, slice_ref(s, slice_len(s) - 1));
    s->len--;
}
