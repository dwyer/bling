#pragma once

#include "slice/slice.h"

package(map);

import("slice");

typedef struct {
    int len;
    int key_size;
    int val_size;
    slice$Slice pairs;
} map$Map;

typedef enum {
    map$status_ok = 1,
} map$status_t;

extern map$Map map$init(int val_size);
extern void map$deinit(map$Map *m);
extern int map$len(const map$Map *m);
extern int map$cap(const map$Map *m);
extern int map$get(const map$Map *m, const char *key, void *val);
extern bool map$has_key(map$Map *m, const char *key);
extern void map$set(map$Map *m, const char *key, const void *val);

extern int map$hits;
extern int map$misses;
extern int map$lookups;
extern int map$iters;

typedef struct {
    const map$Map *_map;
    int _idx;
} map$iter_t;

extern map$iter_t map$iter(const map$Map *m);
extern int map$iter_next(map$iter_t *m, char **key, void *val);
