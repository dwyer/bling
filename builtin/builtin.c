#include "builtin/builtin.h"

static void vprint(char *fmt, va_list ap) {
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

void print(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
}

void panic(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprint(fmt, ap);
    va_end(ap);
    void *buf[1024];
    int n = backtrace(buf, 1024);
    backtrace_symbols_fd(buf, n, 2);
    exit(1);
}

int len(slice_t s) {
    return s.len;
}

int cap(slice_t s) {
    return s.len;
}

void *get_ptr(slice_t s, int index) {
    return &s.array[index * s.size];
}

slice_t append(slice_t s, void *obj) {
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
        s.array = realloc(s.array, s.cap * s.size);
    }
    memcpy(&s.array[s.len * s.size], obj, s.size);
    s.len++;
    return s;
}
