#include "utils/utils.h"

#include "sys/sys.h"

static const float MAP_LOAD_FACTOR = 0.65;
static const int DEFAULT_CAP = 8;

static utils$MapStats stats = {};

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
    return sys$memcpy(malloc(size), src, size);
}

typedef struct {
    void *key;
    void *val;
} MapPair;

extern utils$Map utils$Map_init(int valSize) {
    utils$Map m = {
        ._valSize = valSize,
        ._len = 0,
        ._pairs = utils$Slice_init(sizeof(MapPair)),
    };
    utils$Slice_setLen(&m._pairs, DEFAULT_CAP);
    return m;
}

extern void utils$Map_deinit(utils$Map *m) {
    utils$Slice_deinit(&m->_pairs);
}

extern int utils$Map_len(const utils$Map *m) {
    return m->_len;
}

extern int utils$Map_cap(const utils$Map *m) {
    return utils$Slice_len(&m->_pairs);
}

static MapPair *pair_ref(const utils$Map *m, const void *key) {
    uintptr_t hash = djb2(key) % utils$Map_cap(m);
    stats.lookups++;
    for (int i = 0; i < utils$Slice_len(&m->_pairs); i++) {
        stats.iters++;
        int idx = (hash + i) % utils$Map_cap(m);
        MapPair *p = (MapPair *)utils$Slice_ref(&m->_pairs, idx);
        if (!p->key || streq(key, p->key)) {
            if (!i) {
                stats.hits++;
            } else {
                stats.misses++;
            }
            return p;
        }
    }
    return NULL;
}

static void set_unsafe(utils$Map *m, const char *key, const void *val) {
    MapPair *p = pair_ref(m, key);
    if (p->key == NULL) {
        p->key = strdup(key);
        p->val = memdup(val, m->_valSize);
        m->_len++;
    } else {
        sys$memcpy(p->val, val, m->_valSize);
    }
}

extern bool utils$Map_hasKey(utils$Map *m, const char *key) {
    MapPair *p = pair_ref(m, key);
    return p->key != NULL;
}

extern int utils$Map_get(const utils$Map *m, const char *key, void *val) {
    MapPair *p = pair_ref(m, key);
    if (p->val) {
        sys$memcpy(val, p->val, m->_valSize);
        return 1;
    }
    return 0;
}

extern void utils$Map_set(utils$Map *m, const char *key, const void *val) {
    set_unsafe(m, key, val);
    float load_factor = (float)utils$Map_len(m) / utils$Map_cap(m);
    if (load_factor >= MAP_LOAD_FACTOR) {
        int newCap = utils$Map_cap(m) * 2;
        utils$Slice pairs = m->_pairs;
        m->_pairs = utils$Slice_init(m->_pairs.size);
        utils$Slice_setLen(&m->_pairs, newCap);
        m->_len = 0;
        for (int i = 0; i < utils$Slice_len(&pairs); i++) {
            MapPair *p = (MapPair *)utils$Slice_ref(&pairs, i);
            if (p->key) {
                set_unsafe(m, p->key, p->val);
            }
        }
        utils$Slice_deinit(&pairs);
    }
}

extern bool utils$Map_isInitialized(utils$Map *m) {
    return m->_valSize > 0;
}

extern utils$MapIter utils$NewMapIter(const utils$Map *m) {
    utils$MapIter iter = {._map = m};
    return iter;
}

extern int utils$MapIter_next(utils$MapIter *m, char **key, void *val) {
    while (m->_idx < utils$Slice_len(&m->_map->_pairs)) {
        MapPair *p = (MapPair *)utils$Slice_ref(&m->_map->_pairs, m->_idx);
        m->_idx++;
        if (p->key) {
            if (key) {
                *(void **)key = p->key;
            }
            if (val) {
                sys$memcpy(val, p->val, m->_map->_valSize);
            }
            return 1;
        }
    }
    return 0;
}
