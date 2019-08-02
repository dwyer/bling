#include "slice/slice.h"

extern slice_t slice_init(int size) {
    slice_t s = {
        .size = size,
        .len = 0,
        .cap = 0,
        .array = NULL,
    };
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
    return (void *)&((char *)s->array)[i * s->size];
}

extern void slice_get(const slice_t *s, int i, void *dst) {
    if (i >= s->len) {
        panic("out of range: index=%d, len=%d", i, s->len);
    }
    if (s->size == 1) {
         *(char *)dst = ((char *)s->array)[i];
    } else {
        memcpy(dst, slice_ref(s, i), s->size);
    }
}

static void slice_set_cap(slice_t *s, int cap) {
    s->cap = cap;
    s->array = realloc(s->array, s->cap * s->size);
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
        memset(slice_ref(s, old), 0, diff * s->size);
    }
}

extern void slice_set(slice_t *s, int i, const void *x) {
    if (s->size == 1) {
        ((char *)s->array)[i] = *(char *)x;
    } else {
        memcpy(slice_ref(s, i), x, s->size);
    }
}

extern void slice_append(slice_t *s, const void *x) {
    set_len(s, s->len + 1);
    slice_set(s, s->len - 1, x);
}

extern void *slice_to_nil_array(slice_t s) {
    void *nil = NULL;
    slice_append(&s, &nil);
    return s.array;
}

extern int len(slice_t s) {
    return s.len;
}

extern int cap(slice_t s) {
    return s.cap;
}

extern void *get_ptr(slice_t s, int index) {
    return slice_ref(&s, index);
}

extern slice_t append(slice_t s, const void *obj) {
    slice_append(&s, obj);
    return s;
}
