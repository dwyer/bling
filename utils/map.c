#include "utils/utils.h"

static const float max_load_factor = 0.65;
static const int default_cap = 8;

int utils$Map_misses = 0;
int utils$Map_hits = 0;
int utils$Map_lookups = 0;
int utils$Map_iters = 0;

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

extern utils$Map utils$Map_init(int val_size) {
    utils$Map m = {
        .len = 0,
        .pairs = utils$Slice_init(sizeof(pair_t)),
        .key_size = sizeof(uintptr_t),
        .val_size = val_size,
    };
    utils$Slice_set_len(&m.pairs, default_cap);
    return m;
}

extern void utils$Map_deinit(utils$Map *m) {
    // for (int i = 0; i < utils$Slice_len(&m->pairs); i++) {
    //     pair_t *p = (pair_t *)utils$Slice_ref(&m->pairs, i);
    // }
    utils$Slice_deinit(&m->pairs);
}

extern int utils$Map_len(const utils$Map *m) {
    return m->len;
}

extern int utils$Map_cap(const utils$Map *m) {
    return utils$Slice_len(&m->pairs);
}

static pair_t *pair_ref(const utils$Map *m, const void *key) {
    uintptr_t hash = djb2(key) % utils$Map_cap(m);
    utils$Map_lookups++;
    for (int i = 0; i < utils$Slice_len(&m->pairs); i++) {
        utils$Map_iters++;
        int idx = (hash + i) % utils$Map_cap(m);
        pair_t *p = (pair_t *)utils$Slice_ref(&m->pairs, idx);
        if (!p->key || streq(key, p->key)) {
            if (!i) {
                utils$Map_hits++;
            } else {
                utils$Map_misses++;
            }
            return p;
        }
    }
    return NULL;
}

static void set_unsafe(utils$Map *m, const char *key, const void *val) {
    pair_t *p = pair_ref(m, key);
    if (p->key == NULL) {
        p->key = strdup(key);
        p->val = memdup(val, m->val_size);
        m->len++;
    } else {
        memcpy(p->val, val, m->val_size);
    }
}

extern bool utils$Map_has_key(utils$Map *m, const char *key) {
    pair_t *p = pair_ref(m, key);
    return p->key != NULL;
}

extern int utils$Map_get(const utils$Map *m, const char *key, void *val) {
    pair_t *p = pair_ref(m, key);
    if (p->val) {
        memcpy(val, p->val, m->val_size);
        return 1;
    }
    return 0;
}

extern void utils$Map_set(utils$Map *m, const char *key, const void *val) {
    set_unsafe(m, key, val);
    float load_factor = (float)utils$Map_len(m) / utils$Map_cap(m);
    if (load_factor >= max_load_factor) {
        int new_cap = utils$Map_cap(m) * 2;
        utils$Slice pairs = m->pairs;
        m->pairs = utils$Slice_init(m->pairs.size);
        utils$Slice_set_len(&m->pairs, new_cap);
        m->len = 0;
        for (int i = 0; i < utils$Slice_len(&pairs); i++) {
            pair_t *p = (pair_t *)utils$Slice_ref(&pairs, i);
            if (p->key) {
                set_unsafe(m, p->key, p->val);
            }
        }
        utils$Slice_deinit(&pairs);
    }
}

extern utils$Map_iter_t utils$Map_iter(const utils$Map *m) {
    utils$Map_iter_t iter = {._map = m};
    return iter;
}

extern int utils$Map_iter_next(utils$Map_iter_t *m, char **key, void *val) {
    while (m->_idx < utils$Slice_len(&m->_map->pairs)) {
        pair_t *p = (pair_t *)utils$Slice_ref(&m->_map->pairs, m->_idx);
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
