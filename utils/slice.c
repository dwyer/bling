#include "utils/utils.h"

extern utils$Slice utils$init(int size) {
    utils$Slice s = {
        .size = size,
        .len = 0,
        .cap = 0,
        .array = NULL,
    };
    return s;
}

extern void utils$deinit(utils$Slice *s) {
    free(s->array);
}

extern int utils$len(const utils$Slice *s) {
    return s->len;
}

extern int utils$cap(const utils$Slice *s) {
    return s->cap;
}

extern void *utils$ref(const utils$Slice *s, int i) {
    return (void *)&((char *)s->array)[i * s->size];
}

extern void utils$get(const utils$Slice *s, int i, void *dst) {
    if (i >= s->len) {
        panic("out of range: index=%d, len=%d", i, s->len);
    }
    if (s->size == 1) {
         *(char *)dst = ((char *)s->array)[i];
    } else {
        memcpy(dst, utils$ref(s, i), s->size);
    }
}

static void utils$set_cap(utils$Slice *s, int cap) {
    s->cap = cap;
    s->array = realloc(s->array, s->cap * s->size);
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
        utils$set_cap(s, cap);
    }
    s->len = len;
}

extern void utils$set_len(utils$Slice *s, int len) {
    int old = s->len;
    _set_len(s, len);
    int diff = len - old;
    if (diff > 0) {
        memset(utils$ref(s, old), 0, diff * s->size);
    }
}

extern void utils$set(utils$Slice *s, int i, const void *x) {
    if (s->size == 1) {
        ((char *)s->array)[i] = *(char *)x;
    } else {
        memcpy(utils$ref(s, i), x, s->size);
    }
}

extern void utils$append(utils$Slice *s, const void *x) {
    _set_len(s, s->len + 1);
    utils$set(s, s->len - 1, x);
}

extern void *utils$to_nil_array(utils$Slice s) {
    void *nil = NULL;
    utils$append(&s, &nil);
    return s.array;
}
