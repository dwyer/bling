#include "runtime.h"

extern int desc_cmp(const desc_t *d, const void *a, const void *b) {
    if (d->cmp) {
        if (d->is_ptr) {
            a = *(void **)a;
            b = *(void **)b;
        }
        return d->cmp(a, b);
    }
    return memcmp(a, b, d->size);
}

extern uintptr_t desc_hash(const desc_t *d, const void *p) {
    if (d->hash) {
        if (d->is_ptr) {
            p = *(void **)p;
        }
        return d->hash(p);
    }
    uintptr_t hash = 0;
    int size = sizeof(uintptr_t);
    if (size > d->size) {
        size = d->size;
    }
    memcpy(&hash, p, size);
    return hash;
}
