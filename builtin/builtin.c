#include "builtin/builtin.h"

static void vprint(char *fmt, va_list ap) {
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

extern void print(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
}

extern void panic(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
    void *buf[1024];
    int n = backtrace(buf, 1024);
    backtrace_symbols_fd(buf, n, 2);
    exit(1);
}

extern int len(slice_t s) {
    return s.len;
}

extern int cap(slice_t s) {
    return s.cap;
}

extern void *get_ptr(slice_t s, int index) {
    return &s.array[index * s.desc->size];
}

extern slice_t append(slice_t s, void *obj) {
    bool resize = false;
    if (s.cap == 0) {
        s.cap = 1;
        resize = true;
    } else if (s.cap <= s.len) {
        while (s.cap <= s.len) {
            s.cap *= 2;
        }
        resize = true;
    } else if (s.array == NULL) {
        resize = true;
    }
    if (resize) {
        s.array = realloc(s.array, s.cap * s.desc->size);
    }
    memcpy(&s.array[s.len * s.desc->size], obj, s.desc->size);
    s.len++;
    return s;
}

extern slice_t make_slice(const desc_t *desc, int len, int cap) {
    slice_t slice = slice_init(desc);
    return slice;
}

extern map_t make_map(const desc_t *key_desc, const desc_t *val_desc) {
    map_t map;
    map_init(&map, key_desc, val_desc);
    return map;
}
