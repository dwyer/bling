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
    slice_append(&s, obj);
    return s;
}

extern slice_t make_slice(const desc_t *desc, int len, int cap) {
    return slice_init(desc, len, cap);
}

extern map_t make_map(const desc_t *key_desc, const desc_t *val_desc) {
    return map_init(key_desc, val_desc);
}
