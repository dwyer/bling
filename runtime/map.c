#include "runtime.h"

#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <stdio.h>

static const float max_load_factor = 0.65;

int map_misses = 0;
int map_hits = 0;
int map_lookups = 0;
int map_iters = 0;

typedef struct {
    void *key, *val;
} pair_t;

static void pair_deinit(void *p) {
    pair_t *pp = p;
    free(pp->key);
    free(pp->val);
}

static const desc_t pair_desc = {
    .size = sizeof(pair_t),
    .deinit = pair_deinit,
};

extern void map_init(map_t *m, const void *key_desc, const void *val_desc) {
    m->len = 0;
    m->pairs = slice_init(&pair_desc);
    m->key_desc = key_desc;
    m->val_desc = val_desc;
    slice_set_len(&m->pairs, 8);
}

extern void map_deinit(map_t *m) {
    slice_deinit(&m->pairs);
}

extern int map_len(const map_t *m) {
    return m->len;
}

extern int map_cap(const map_t *m) {
    return slice_len(&m->pairs);
}

static void *pair_ref(const map_t *m, const void *key) {
    unsigned int hash = desc_hash(m->key_desc, key) % map_cap(m);
    ++map_lookups;
    for (int i = 0; i < slice_len(&m->pairs); i++) {
        ++map_iters;
        unsigned int idx = (hash + i) % map_cap(m);
        pair_t *p = slice_ref(&m->pairs, idx);
        if (!p->key || !desc_cmp(m->key_desc, key, p->key)) {
            if (!i)
                ++map_hits;
            else
                ++map_misses;
            return p;
        }
    }
    return NULL;
}

static void set_unsafe(map_t *m, const void *key, const void *val) {
    pair_t *p = pair_ref(m, key);
    if (p->key == NULL) {
        p->key = malloc(m->key_desc->size);
        p->val = malloc(m->val_desc->size);
        desc_cpy(m->key_desc, p->key, key);
        desc_cpy(m->val_desc, p->val, val);
        m->len++;
    } else {
        desc_deinit(m->val_desc, p->val);
        desc_cpy(m->val_desc, p->val, val);
    }
}

static void *val_ref(const map_t *m, const void *key) {
    pair_t *p = pair_ref(m, key);
    if (p)
        return p->val;
    return NULL;
}

extern int map_get(const map_t *m, const void *key, void *val) {
    void *ref = val_ref(m, key);
    if (ref) {
        memcpy(val, ref, m->val_desc->size);
        return 0;
    }
    return 1;
}

extern void map_set(map_t *m, const void *key, const void *val) {
    set_unsafe(m, key, val);
    float load_factor = (float)map_len(m) / map_cap(m);
    if (load_factor >= max_load_factor) {
        int new_cap = map_cap(m) * 2;
        slice_t pairs = slice_init(&pair_desc);
        while (slice_len(&m->pairs)) {
            pair_t p;
            slice_pop(&m->pairs, &p);
            if (p.key)
                slice_append(&pairs, &p);
        }
        assert(map_cap(m) == 0);
        slice_set_len(&m->pairs, new_cap);
        m->len = 0;
        for (int i = 0; i < slice_len(&pairs); ++i) {
            pair_t *p = slice_ref(&pairs, i);
            set_unsafe(m, p->key, p->val);
        }
        slice_deinit(&pairs);
    }
}

extern slice_t map_keys(const map_t *m) {
    slice_t keys = slice_init(m->key_desc);
    for (int i = 0; i < slice_len(&m->pairs); i++) {
        pair_t *p = slice_ref(&m->pairs, i);
        if (p->key)
            slice_append(&keys, p->key);
    }
    return keys;
}
