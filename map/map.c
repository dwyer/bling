#include "map/map.h"

static const float max_load_factor = 0.65;
static const int default_cap = 8;

int map_misses = 0;
int map_hits = 0;
int map_lookups = 0;
int map_iters = 0;

static uintptr_t djb2(const char *s) {
    // http://www.cse.yorku.ca/~oz/hash.html
    uintptr_t hash = 5381;
    int ch = *s;
    while (ch) {
        hash = ((hash << 5) + hash) + ch;
        s++;
        ch = *s;
    }
    return hash;
}

static void *memdup(const void *src, size_t size) {
    return memcpy(malloc(size), src, size);
}

typedef struct {
    void *key;
    void *val;
} pair_t;

extern map_t map_init(int val_size) {
    map_t m = {
        .len = 0,
        .pairs = slice_init(sizeof(pair_t), default_cap, 0),
        .key_size = sizeof(uintptr_t),
        .val_size = val_size,
    };
    return m;
}

extern void map_deinit(map_t *m) {
    // for (int i = 0; i < slice_len(&m->pairs); i++) {
    //     pair_t *p = (pair_t *)slice_ref(&m->pairs, i);
    // }
    slice_deinit(&m->pairs);
}

extern int map_len(const map_t *m) {
    return m->len;
}

extern int map_cap(const map_t *m) {
    return slice_len(&m->pairs);
}

static pair_t *pair_ref(const map_t *m, const void *key) {
    uintptr_t hash = djb2(key) % map_cap(m);
    map_lookups++;
    for (int i = 0; i < slice_len(&m->pairs); i++) {
        map_iters++;
        int idx = (hash + i) % map_cap(m);
        pair_t *p = (pair_t *)slice_ref(&m->pairs, idx);
        if (!p->key || !strcmp(key, p->key)) {
            if (!i) {
                map_hits++;
            } else {
                map_misses++;
            }
            return p;
        }
    }
    return NULL;
}

static void set_unsafe(map_t *m, const void *key, const void *val) {
    pair_t *p = pair_ref(m, key);
    if (p->key == NULL) {
        p->key = strdup(key);
        p->val = memdup(val, m->val_size);
        m->len++;
    } else {
        memcpy(p->val, val, m->val_size);
    }
}

extern bool map_has_key(map_t *m, const void *key) {
    pair_t *p = pair_ref(m, key);
    return p->key != NULL;
}

extern int map_get(const map_t *m, const void *key, void *val) {
    pair_t *p = pair_ref(m, key);
    if (p->val) {
        memcpy(val, p->val, m->val_size);
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
        m->pairs = slice_init(m->pairs.size, new_cap, 0);
        m->len = 0;
        for (int i = 0; i < slice_len(&pairs); i++) {
            pair_t *p = (pair_t *)slice_ref(&pairs, i);
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
        pair_t *p = (pair_t *)slice_ref(&m->_map->pairs, m->_idx);
        m->_idx++;
        if (p->key) {
            if (key) {
                *(void **)key = p->key;
            }
            if (val) {
                memcpy(val, p->val, m->_map->val_size);
            }
            return 1;
        }
    }
    return 0;
}
