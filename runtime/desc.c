#include "runtime.h"

#include <stdlib.h>
#include <string.h>

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

extern uint32_t desc_hash(const desc_t *d, const void *p) {
    if (d->hash) {
        if (d->is_ptr) {
            p = *(void **)p;
        }
        return d->hash(p);
    }
    unsigned hash = 0;
    int size = min(d->size, sizeof(unsigned int));
    memcpy(&hash, p, size);
    return hash;
}
