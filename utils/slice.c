#include "utils/utils.h"

extern utils$Slice_Slice utils$Slice_init(int size) {
    utils$Slice_Slice s = {
        .size = size,
        .len = 0,
        .cap = 0,
        .array = NULL,
    };
    return s;
}

extern void utils$Slice_deinit(utils$Slice_Slice *s) {
    free(s->array);
}

extern int utils$Slice_len(const utils$Slice_Slice *s) {
    return s->len;
}

extern int utils$Slice_cap(const utils$Slice_Slice *s) {
    return s->cap;
}

extern void *utils$Slice_ref(const utils$Slice_Slice *s, int i) {
    return (void *)&((char *)s->array)[i * s->size];
}

extern void utils$Slice_get(const utils$Slice_Slice *s, int i, void *dst) {
    if (i >= s->len) {
        panic("out of range: index=%d, len=%d", i, s->len);
    }
    if (s->size == 1) {
         *(char *)dst = ((char *)s->array)[i];
    } else {
        memcpy(dst, utils$Slice_ref(s, i), s->size);
    }
}

static void utils$Slice_set_cap(utils$Slice_Slice *s, int cap) {
    s->cap = cap;
    s->array = realloc(s->array, s->cap * s->size);
}

static void _set_len(utils$Slice_Slice *s, int len) {
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
        utils$Slice_set_cap(s, cap);
    }
    s->len = len;
}

extern void utils$Slice_set_len(utils$Slice_Slice *s, int len) {
    int old = s->len;
    _set_len(s, len);
    int diff = len - old;
    if (diff > 0) {
        memset(utils$Slice_ref(s, old), 0, diff * s->size);
    }
}

extern void utils$Slice_set(utils$Slice_Slice *s, int i, const void *x) {
    if (s->size == 1) {
        ((char *)s->array)[i] = *(char *)x;
    } else {
        memcpy(utils$Slice_ref(s, i), x, s->size);
    }
}

extern void utils$Slice_append(utils$Slice_Slice *s, const void *x) {
    _set_len(s, s->len + 1);
    utils$Slice_set(s, s->len - 1, x);
}

extern void *utils$Slice_to_nil_array(utils$Slice_Slice s) {
    void *nil = NULL;
    utils$Slice_append(&s, &nil);
    return s.array;
}
