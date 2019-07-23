#pragma once

#include "slice/slice.h"

import("slice");

typedef struct {
    int len;
    int key_size;
    int val_size;
    slice_t pairs;
} map_t;

typedef enum {
    map_status_ok = 1,
} map_status_t;

extern map_t map_init(int val_size);
extern void map_deinit(map_t *m);
extern int map_len(const map_t *m);
extern int map_cap(const map_t *m);
extern int map_get(const map_t *m, const char *key, void *val);
extern bool map_has_key(map_t *m, const char *key);
extern void map_set(map_t *m, const char *key, const void *val);

extern int map_hits;
extern int map_misses;
extern int map_lookups;
extern int map_iters;

typedef struct {
    const map_t *_map;
    int _idx;
} map_iter_t;

extern map_iter_t map_iter(const map_t *m);
extern int map_iter_next(map_iter_t *m, char **key, void *val);
