#include "utils/utils.h"

#include "sys/sys.h"

extern utils$Slice utils$Slice_make(int size) {
    utils$Slice s = {
        .size = size,
        ._len = 0,
        .cap = 0,
        ._array = NULL,
    };
    return s;
}

extern void utils$Slice_unmake(void *s) {
    sys$free(((utils$Slice *)s)->_array);
}

extern int utils$Slice_len(const void *s) {
    return ((utils$Slice *)s)->_len;
}

extern int utils$Slice_cap(const void *s) {
    return ((utils$Slice *)s)->cap;
}

extern void *utils$Slice_get(const void *s, int i, void *dst) {
    if (i >= ((utils$Slice *)s)->_len) {
        panic(sys$sprintf("out of range: index=%d, len=%d", i, ((utils$Slice *)s)->_len));
    }
    char *ref = &((char *)((utils$Slice *)s)->_array)[i * ((utils$Slice *)s)->size];
    if (dst) {
        if (((utils$Slice *)s)->size == 1) {
            *(char *)dst = *ref;
        } else {
            sys$memcpy(dst, ref, ((utils$Slice *)s)->size);
        }
    }
    return ref;
}

static void utils$_setCap(utils$Slice *s, int cap) {
    s->cap = cap;
    s->_array = sys$realloc(s->_array, s->cap * s->size);
}

static void _setLen(utils$Slice *s, int len) {
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
    if (s->_array == NULL || grow) {
        utils$_setCap(s, cap);
    }
    s->_len = len;
}

extern void utils$Slice_setLen(void *s, int len) {
    int old = ((utils$Slice *)s)->_len;
    _setLen(s, len);
    int diff = len - old;
    if (diff > 0) {
        sys$memset(utils$Slice_get(s, old, NULL), 0, diff * ((utils$Slice *)s)->size);
    }
}

extern void utils$Slice_set(void *s, int i, const void *x) {
    if (((utils$Slice *)s)->size == 1) {
        ((char *)((utils$Slice *)s)->_array)[i] = *(char *)x;
    } else {
        sys$memcpy(utils$Slice_get(s, i, NULL), x, ((utils$Slice *)s)->size);
    }
}

extern void utils$Slice_append(void *s, const void *x) {
    _setLen(s, ((utils$Slice *)s)->_len + 1);
    utils$Slice_set(s, ((utils$Slice *)s)->_len - 1, x);
}

extern void *utils$nilArray(void *s) {
    void *nil = NULL;
    utils$Slice_append(s, &nil);
    return ((utils$Slice *)s)->_array;
}
