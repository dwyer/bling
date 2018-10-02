#include "builtin/builtin.h"

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
