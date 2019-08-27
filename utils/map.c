#include "utils/utils.h"

#include "sys/sys.h"

static const float MAP_LOAD_FACTOR = 0.65;
static const int DEFAULT_CAP = 8;

static utils$MapStats stats = {};

static uintptr djb2(const char *s) {
    // http://www.cse.yorku.ca/~oz/hash.html
    uintptr hash = 5381;
    int ch = *s;
    while (ch) {
        hash = ((hash << 5) + hash) + ch;
        s++;
        ch = *s;
    }
    return hash;
}

static void *memdup(const void *src, sys$Size size) {
    return sys$memcpy(sys$malloc(size), src, size);
}

typedef struct {
    void *key;
    void *val;
} MapPair;

extern utils$Map utils$Map_make(int valSize) {
    utils$Map m = {
        ._valSize = valSize,
        ._len = 0,
        ._pairs = utils$Slice_make(sizeof(MapPair)),
    };
    utils$Slice_setLen(&m._pairs, DEFAULT_CAP);
    return m;
}

extern void utils$Map_unmake(void *m) {
    utils$Slice_unmake(&((utils$Map *)m)->_pairs);
}

extern int utils$Map_len(const void *m) {
    return ((utils$Map *)m)->_len;
}

extern int utils$Map_cap(const void *m) {
    return len(((utils$Map *)m)->_pairs);
}

static MapPair *pair_ref(const utils$Map *m, const void *key) {
    uintptr hash = djb2(key) % utils$Map_cap(m);
    stats.lookups++;
    for (int i = 0; i < len(m->_pairs); i++) {
        stats.iters++;
        int idx = (hash + i) % utils$Map_cap(m);
        MapPair *p = utils$Slice_get(&m->_pairs, idx, NULL);
        if (!p->key || sys$streq(key, p->key)) {
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
        p->key = sys$strdup(key);
        p->val = memdup(val, m->_valSize);
        m->_len++;
    } else {
        sys$memcpy(p->val, val, m->_valSize);
    }
}

extern bool utils$Map_get(const void *m, const char *key, void *val) {
    MapPair *p = pair_ref(m, key);
    if (p->val) {
        if (val) {
            sys$memcpy(val, p->val, ((utils$Map *)m)->_valSize);
        }
        return true;
    }
    return false;
}

extern void utils$Map_set(void *m, const char *key, const void *val) {
    set_unsafe(m, key, val);
    float load_factor = (float)utils$Map_len(m) / utils$Map_cap(m);
    if (load_factor >= MAP_LOAD_FACTOR) {
        int newCap = utils$Map_cap(m) * 2;
        utils$Slice pairs = ((utils$Map *)m)->_pairs;
        ((utils$Map *)m)->_pairs = utils$Slice_make(((utils$Map *)m)->_pairs.size);
        utils$Slice_setLen(&((utils$Map *)m)->_pairs, newCap);
        ((utils$Map *)m)->_len = 0;
        for (int i = 0; i < len(pairs); i++) {
            MapPair *p = utils$Slice_get(&pairs, i, NULL);
            if (p->key) {
                set_unsafe(((utils$Map *)m), p->key, p->val);
            }
        }
        utils$Slice_unmake(&pairs);
    }
}

extern utils$MapIter utils$NewMapIter(const void *m) {
    utils$MapIter iter = {._map = m};
    return iter;
}

extern int utils$MapIter_next(utils$MapIter *m, char **key, void *val) {
    while (m->_idx < len(m->_map->_pairs)) {
        MapPair *p = (MapPair *)utils$Slice_get(&m->_map->_pairs, m->_idx, NULL);
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
