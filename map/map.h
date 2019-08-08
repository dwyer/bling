#pragma once

#include "utils/utils.h"

package(map);

import("utils");

typedef struct {
    int len;
    int key_size;
    int val_size;
    utils$Slice pairs;
} map$Map;

typedef enum {
    map$Map_status_ok = 1,
} map$Map_status_t;

extern map$Map map$Map_init(int val_size);
extern void map$Map_deinit(map$Map *m);
extern int map$Map_len(const map$Map *m);
extern int map$Map_cap(const map$Map *m);
extern int map$Map_get(const map$Map *m, const char *key, void *val);
extern bool map$Map_has_key(map$Map *m, const char *key);
extern void map$Map_set(map$Map *m, const char *key, const void *val);

extern int map$Map_hits;
extern int map$Map_misses;
extern int map$Map_lookups;
extern int map$Map_iters;

typedef struct {
    const map$Map *_map;
    int _idx;
} map$Map_iter_t;

extern map$Map_iter_t map$Map_iter(const map$Map *m);
extern int map$Map_iter_next(map$Map_iter_t *m, char **key, void *val);
