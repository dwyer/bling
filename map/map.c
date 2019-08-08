#include "map/map.h"

static const float max_load_factor = 0.65;
static const int default_cap = 8;

int map$misses = 0;
int map$hits = 0;
int map$lookups = 0;
int map$iters = 0;

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

extern map$Map map$init(int val_size) {
    map$Map m = {
        .len = 0,
        .pairs = slice$init(sizeof(pair_t)),
        .key_size = sizeof(uintptr_t),
        .val_size = val_size,
    };
    slice$set_len(&m.pairs, default_cap);
    return m;
}

extern void map$deinit(map$Map *m) {
    // for (int i = 0; i < slice$len(&m->pairs); i++) {
    //     pair_t *p = (pair_t *)slice$ref(&m->pairs, i);
    // }
    slice$deinit(&m->pairs);
}

extern int map$len(const map$Map *m) {
    return m->len;
}

extern int map$cap(const map$Map *m) {
    return slice$len(&m->pairs);
}

static pair_t *pair_ref(const map$Map *m, const void *key) {
    uintptr_t hash = djb2(key) % map$cap(m);
    map$lookups++;
    for (int i = 0; i < slice$len(&m->pairs); i++) {
        map$iters++;
        int idx = (hash + i) % map$cap(m);
        pair_t *p = (pair_t *)slice$ref(&m->pairs, idx);
        if (!p->key || streq(key, p->key)) {
            if (!i) {
                map$hits++;
            } else {
                map$misses++;
            }
            return p;
        }
    }
    return NULL;
}

static void set_unsafe(map$Map *m, const char *key, const void *val) {
    pair_t *p = pair_ref(m, key);
    if (p->key == NULL) {
        p->key = strdup(key);
        p->val = memdup(val, m->val_size);
        m->len++;
    } else {
        memcpy(p->val, val, m->val_size);
    }
}

extern bool map$has_key(map$Map *m, const char *key) {
    pair_t *p = pair_ref(m, key);
    return p->key != NULL;
}

extern int map$get(const map$Map *m, const char *key, void *val) {
    pair_t *p = pair_ref(m, key);
    if (p->val) {
        memcpy(val, p->val, m->val_size);
        return 1;
    }
    return 0;
}

extern void map$set(map$Map *m, const char *key, const void *val) {
    set_unsafe(m, key, val);
    float load_factor = (float)map$len(m) / map$cap(m);
    if (load_factor >= max_load_factor) {
        int new_cap = map$cap(m) * 2;
        slice$Slice pairs = m->pairs;
        m->pairs = slice$init(m->pairs.size);
        slice$set_len(&m->pairs, new_cap);
        m->len = 0;
        for (int i = 0; i < slice$len(&pairs); i++) {
            pair_t *p = (pair_t *)slice$ref(&pairs, i);
            if (p->key) {
                set_unsafe(m, p->key, p->val);
            }
        }
        slice$deinit(&pairs);
    }
}

extern map$iter_t map$iter(const map$Map *m) {
    map$iter_t iter = {._map = m};
    return iter;
}

extern int map$iter_next(map$iter_t *m, char **key, void *val) {
    while (m->_idx < slice$len(&m->_map->pairs)) {
        pair_t *p = (pair_t *)slice$ref(&m->_map->pairs, m->_idx);
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
