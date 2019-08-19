#include "utils/utils.h"

#include "sys/sys.h"

extern utils$Slice utils$Slice_init(int size) {
    utils$Slice s = {
        .size = size,
        .len = 0,
        .cap = 0,
        .array = NULL,
    };
    return s;
}

extern void utils$Slice_deinit(utils$Slice *s) {
    free(s->array);
}

extern int utils$Slice_len(const utils$Slice *s) {
    return s->len;
}

extern int utils$Slice_cap(const utils$Slice *s) {
    return s->cap;
}

extern void *utils$Slice_ref(const utils$Slice *s, int i) {
    return &((char *)s->array)[i * s->size];
}

extern void utils$Slice_get(const utils$Slice *s, int i, void *dst) {
    if (i >= s->len) {
        panic("out of range: index=%d, len=%d", i, s->len);
    }
    if (s->size == 1) {
         *(char *)dst = ((char *)s->array)[i];
    } else {
        sys$memcpy(dst, utils$Slice_ref(s, i), s->size);
    }
}

static void utils$Slice_set_cap(utils$Slice *s, int cap) {
    s->cap = cap;
    s->array = sys$realloc(s->array, s->cap * s->size);
}

static void _set_len(utils$Slice *s, int len) {
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

extern void utils$Slice_set_len(utils$Slice *s, int len) {
    int old = s->len;
    _set_len(s, len);
    int diff = len - old;
    if (diff > 0) {
        sys$memset(utils$Slice_ref(s, old), 0, diff * s->size);
    }
}

extern void utils$Slice_set(utils$Slice *s, int i, const void *x) {
    if (s->size == 1) {
        ((char *)s->array)[i] = *(char *)x;
    } else {
        sys$memcpy(utils$Slice_ref(s, i), x, s->size);
    }
}

extern void utils$Slice_append(utils$Slice *s, const void *x) {
    _set_len(s, s->len + 1);
    utils$Slice_set(s, s->len - 1, x);
}

extern void *utils$Slice_to_nil_array(utils$Slice s) {
    void *nil = NULL;
    utils$Slice_append(&s, &nil);
    return s.array;
}
