#include "runtime.h"

#include <stdlib.h>
#include <string.h>

static void *memdup(const void *src, size_t size) {
    return memcpy(malloc(size), src, size);
}

extern void desc_deinit(const desc_t *d, void *x) {
    if (d->deinit)
        d->deinit(x);
}

extern void desc_cpy(const desc_t *d, void *dst, const void *src) {
    if (d->cpy)
        d->cpy(dst, src);
    else
        memcpy(dst, src, d->size);
}

extern int desc_cmp(const desc_t *d, const void *a, const void *b) {
    if (d->cmp)
        return d->cmp(a, b);
    return memcmp(a, b, d->size);
}

extern uint32_t desc_hash(const desc_t *d, const void *p) {
    if (d->hash)
        return d->hash(p);
    unsigned hash = 0;
    int size = min(d->size, sizeof(unsigned int));
    memcpy(&hash, p, size);
    return hash;
}
