#include "runtime.h"

static const float max_load_factor = 0.65;
static const int default_cap = 8;

int map_misses = 0;
int map_hits = 0;
int map_lookups = 0;
int map_iters = 0;

typedef struct {
    void *key, *val;
} pair_t;

extern map_t map_init(const void *key_desc, const void *val_desc) {
    static const desc_t pair_desc = {
        .size = sizeof(pair_t),
    };
    map_t m = {
        .len = 0,
        .pairs = slice_init(&pair_desc, default_cap, 0),
        .key_desc = key_desc,
        .val_desc = val_desc,
    };
    return m;
}

extern void map_deinit(map_t *m) {
    for (int i = 0; i < slice_len(&m->pairs); i++) {
        pair_t *p = slice_ref(&m->pairs, i);
    }
    slice_deinit(&m->pairs);
}

extern int map_len(const map_t *m) {
    return m->len;
}

extern int map_cap(const map_t *m) {
    return slice_len(&m->pairs);
}

static pair_t *pair_ref(const map_t *m, const void *key) {
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
        memcpy(p->key, key, m->key_desc->size);
        memcpy(p->val, val, m->val_desc->size);
        m->len++;
    } else {
        memcpy(p->val, val, m->val_desc->size);
    }
}

extern bool map_has_key(map_t *m, const void *key) {
    pair_t *p = pair_ref(m, key);
    return p->key;
}

extern int map_get(const map_t *m, const void *key, void *val) {
    pair_t *p = pair_ref(m, key);
    if (p->val) {
        memcpy(val, p->val, m->val_desc->size);
        return 1;
    }
    return 0;
}

extern void map_set(map_t *m, const void *key, const void *val) {
    set_unsafe(m, key, val);
    float load_factor = (float)map_len(m) / map_cap(m);
    if (load_factor >= max_load_factor) {
        int new_cap = map_cap(m) * 2;
        slice_t pairs = m->pairs;
        m->pairs = slice_init(m->pairs.desc, new_cap, 0);
        m->len = 0;
        for (int i = 0; i < slice_len(&pairs); ++i) {
            pair_t *p = slice_ref(&pairs, i);
            if (p->key) {
                set_unsafe(m, p->key, p->val);
            }
        }
        slice_deinit(&pairs);
    }
}

extern map_iter_t map_iter(const map_t *m) {
    map_iter_t iter = {._map = m};
    return iter;
}

extern int map_iter_next(map_iter_t *m, void *key, void *val) {
    while (m->_idx < slice_len(&m->_map->pairs)) {
        pair_t *p = slice_ref(&m->_map->pairs, m->_idx);
        m->_idx++;
        if (p->key) {
            if (key) memcpy(key, p->key, m->_map->key_desc->size);
            if (val) memcpy(val, p->val, m->_map->val_desc->size);
            return 1;
        }
    }
    return 0;
}
